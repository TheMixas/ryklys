// Chat fan-out benchmark for Ryklys.
//
// Opens N viewer WebSockets on one stream, then one sender posts M chat
// messages. Each message goes through the real path: WebSocket -> backend ->
// PostgreSQL insert -> Redis PUBLISH -> ChatRelay -> broadcast to every viewer.
// For every viewer and message we record send-to-receive time.
//
// Usage: node ws-fanout.mjs [viewers=1000] [messages=50] [host=127.0.0.1:3001]
import WebSocket from "ws";

const VIEWERS = Number(process.argv[2] ?? 1000);
const MESSAGES = Number(process.argv[3] ?? 50);
const HOST = process.argv[4] ?? "127.0.0.1:3001";
const STREAM = `bench-${Date.now()}`;
const GAP_MS = 200; // time between chat messages

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const nowMs = () => Number(process.hrtime.bigint()) / 1e6;

async function getToken() {
    const id = Date.now().toString(36);
    const body = JSON.stringify({ username: `bench_${id}`, email: `bench_${id}@example.com`, password: "Bench12345" });
    const res = await fetch(`http://${HOST}/api/users/register`, {
        method: "POST", headers: { "Content-Type": "application/json" }, body,
    });
    const cookie = res.headers.get("set-cookie") ?? "";
    const m = cookie.match(/token=([^;]+)/);
    if (!m) throw new Error(`register failed: ${res.status} ${await res.text()}`);
    return m[1];
}

function open(token) {
    return new Promise((resolve, reject) => {
        const ws = new WebSocket(`ws://${HOST}/ws/view?streamId=${STREAM}`, {
            headers: { Cookie: `token=${token}` },
            perMessageDeflate: false,
        });
        ws.once("open", () => resolve(ws));
        ws.once("error", reject);
    });
}

const percentile = (sorted, p) => sorted[Math.min(sorted.length - 1, Math.floor((p / 100) * sorted.length))];

async function main() {
    const token = await getToken();
    console.log(`stream ${STREAM}: opening ${VIEWERS} viewers...`);

    // sendTimes[msgIndex] = when it was sent; latencies collects receive - send for every delivery
    const sendTimes = new Map();
    const latencies = [];
    const lastArrival = new Map(); // msgIndex -> latest arrival (fan-out completion)
    let received = 0;

    const onMessage = (data) => {
        const t = nowMs();
        const msg = JSON.parse(data.toString());
        if (msg.type !== "chat" || !msg.text.startsWith("bench:")) return;
        const i = Number(msg.text.slice(6));
        const sent = sendTimes.get(i);
        if (sent === undefined) return;
        latencies.push(t - sent);
        lastArrival.set(i, Math.max(lastArrival.get(i) ?? 0, t));
        received++;
    };

    const t0 = nowMs();
    const viewers = [];
    const BATCH = 100;
    for (let i = 0; i < VIEWERS; i += BATCH) {
        const batch = await Promise.all(
            Array.from({ length: Math.min(BATCH, VIEWERS - i) }, () => open(token)),
        );
        for (const ws of batch) ws.on("message", onMessage);
        viewers.push(...batch);
    }
    const connectSecs = (nowMs() - t0) / 1000;
    console.log(`connected ${viewers.length} viewers in ${connectSecs.toFixed(1)} s`);

    // Let the viewer-count broadcasts from all the joins finish
    await sleep(3000);

    const sender = await open(token);
    await sleep(500); // the server attaches its message handler just after the upgrade
    for (let i = 0; i < MESSAGES; i++) {
        sendTimes.set(i, nowMs());
        sender.send(JSON.stringify({ type: "chat", text: `bench:${i}` }));
        await sleep(GAP_MS);
    }
    await sleep(3000); // wait for stragglers

    const expected = MESSAGES * VIEWERS;
    latencies.sort((a, b) => a - b);
    const completion = [...lastArrival.entries()].map(([i, t]) => t - sendTimes.get(i)).sort((a, b) => a - b);

    console.log(`\nviewers            ${VIEWERS}`);
    console.log(`messages           ${MESSAGES}`);
    console.log(`delivered          ${received} / ${expected} (${((100 * received) / expected).toFixed(2)}%)`);
    if (latencies.length) {
        console.log(`delivery latency   p50 ${percentile(latencies, 50).toFixed(1)} ms | p99 ${percentile(latencies, 99).toFixed(1)} ms | max ${latencies.at(-1).toFixed(1)} ms`);
        console.log(`last viewer gets   p50 ${percentile(completion, 50).toFixed(1)} ms | p99 ${percentile(completion, 99).toFixed(1)} ms`);
    }

    for (const ws of viewers) ws.terminate();
    sender.terminate();
}

main().catch((e) => {
    console.error(e);
    process.exit(1);
});
