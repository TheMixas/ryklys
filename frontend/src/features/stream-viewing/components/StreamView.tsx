import {
    Heading,
    Text,
    Flex,
    Button,
    Badge,
    Avatar,
    Box,
    ScrollArea,
    TextField,
    IconButton,
    Separator
} from '@radix-ui/themes';
import {useState, useRef, useEffect} from 'react';
import {PaperPlaneIcon, HeartIcon, PersonIcon, ChatBubbleIcon} from '@radix-ui/react-icons';
import Hls from 'hls.js';
import {env} from "@/config/env.ts";
import {useParams} from "react-router";

// --- Stub types / hooks (replace with your real ones) ---
interface ChatMessage {
    id: string;
    username: string;
    text: string;
    createdAt: string;
}


const usernameColor = (name: string): string => {
    const colors = ['#e05c5c', '#5c9ce0', '#5ce09a', '#c45ce0', '#e0a85c', '#5ce0d4'];
    let hash = 0;
    for (const ch of name) hash = ch.charCodeAt(0) + ((hash << 5) - hash);
    return colors[Math.abs(hash) % colors.length];
};

const StreamView = () => {
    const videoRef = useRef<HTMLVideoElement>(null);
    const hlsRef = useRef<Hls | null>(null);
    const chatBottomRef = useRef<HTMLDivElement>(null);
    const wsRef = useRef<WebSocket | null>(null);
    const [messages, setMessages] = useState<ChatMessage[]>([]);
    const [inputValue, setInputValue] = useState('');
    const [isFollowing, setIsFollowing] = useState(false);
    const [viewerCount, setViewerCount] = useState(142);
    const [isLive, setIsLive] = useState(false);
    const [error, setError] = useState<string | null>(null);

    // Stub: replace with your real WS stream consumption
    const stream: MediaStream | null = null;

    //Params
    const {username} = useParams();
    //fetch chat messages
    useEffect(() => {
        if (!username) return;

        const fetchHistory = async () => {
            try {
                const res = await fetch(
                    `${env.API_URL}/api/streams/${username}/chat?limit=50`
                );
                if (!res.ok) return;
                const data: ChatMessage[] = await res.json();
                setMessages(data);
            } catch (e) {
                console.error('[Chat] Failed to fetch history:', e);
            }
        };

        fetchHistory();
    }, [username]);
    // HLS.js setup
    useEffect(() => {
        const video = videoRef.current;
        if (!video || !username) return;

        const streamUrl = `${env.STREAM_POCKET_URL}/api/segments/${username}/${username}.m3u8`;
        console.log("Stream url for hls: " + streamUrl);
        if (Hls.isSupported()) {
            console.log("Hls is supported, starting HLS.")
            const hls = new Hls({
                liveDurationInfinity: true,
                lowLatencyMode: true,
                backBufferLength: 30,
            });

            hls.loadSource(streamUrl);
            hls.attachMedia(video);

            hls.on(Hls.Events.MANIFEST_PARSED, () => {
                setIsLive(true);
                setError(null);
                video.play().catch(() => {
                });
            });

            hls.on(Hls.Events.ERROR, (_event, data) => {
                if (data.fatal) {
                    if (data.type === Hls.ErrorTypes.NETWORK_ERROR) {
                        setError('Stream not available yet');
                        // Retry after a few seconds
                        setTimeout(() => hls.loadSource(streamUrl), 3000);
                    } else {
                        setError('Playback error');
                        hls.destroy();
                    }
                }
            });

            hlsRef.current = hls;

            return () => {
                hls.destroy();
                hlsRef.current = null;
            };
        } else if (video.canPlayType('application/vnd.apple.mpegurl')) {
            console.log("Hls is unsupported, using safari native.")
            // Safari native HLS
            video.src = streamUrl;
            video.addEventListener('loadedmetadata', () => {
                setIsLive(true);
                video.play().catch(() => {
                });
            });
        }
    }, [username]);
    // Viewer WebSocket — tracks viewer count
    useEffect(() => {
        if (!username) return;

        const wsUrl = `${env.API_URL.replace(/^http/, 'ws')}/ws/view?streamId=${username}`;
        const ws = new WebSocket(wsUrl);
        wsRef.current = ws; // store ref for sending (see substep 4)

        ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);

                switch (data.type) {
                    case 'viewer_count':
                        setViewerCount(data.count);
                        break;

                    case 'chat':
                        setMessages(prev => {
                            if (prev.some(m => m.id === data.id)) return prev;
                            return [...prev, { id: data.id, username: data.username, text: data.text, createdAt: data.createdAt }];
                        });
                        break;
                }
            } catch (e) {
                console.error('[WS] parse error:', e);
            }
        };

        ws.onclose = (event) => {
            console.log('[WS] disconnected', event.code, event.reason);
            wsRef.current = null;

            if (event.code === 1001 && event.reason === 'Stream ended') {
                setIsLive(false);
                setError('Stream has ended');
            }
        };

        return () => ws.close();
    }, [username]);
    useEffect(() => {
        if (stream && videoRef.current) {
            videoRef.current.srcObject = stream;
        }
    }, [stream]);

    useEffect(() => {
        chatBottomRef.current?.scrollIntoView({behavior: 'smooth'});
    }, [messages]);

    const handleSendMessage = () => {
        const text = inputValue.trim();
        if (!text || !wsRef.current || wsRef.current.readyState !== WebSocket.OPEN) return;

        wsRef.current.send(JSON.stringify({ type: 'chat', text }));
        console.log('[WS] sendMessage', text);
        setInputValue('');
    };

    return (
        <Flex direction="column" gap="4" p="6" width="100%">

            {/* Page Header */}
            <Heading size="8">Stream</Heading>
            <Text color="gray" size="4">
                Watch live streams from your favourite creators.
            </Text>

            {/* Main content row — mirrors StreamerDashboard's two-column split */}
            <Flex direction="row" gap="6" align="start" justify="between" width="100%">

                {/* LEFT — Video + streamer info */}
                <Flex direction="column" id="VideoHalf" gap="4" width="100%" style={{flex: '1 1 0'}}>

                    {/* Video player */}
                    {/*<Box*/}
                    {/*    style={{*/}
                    {/*        width: '100%',*/}
                    {/*        aspectRatio: '16/9',*/}
                    {/*        background: 'var(--gray-3)',*/}
                    {/*        borderRadius: 'var(--radius-3)',*/}
                    {/*        overflow: 'hidden',*/}
                    {/*        position: 'relative',*/}
                    {/*    }}*/}
                    {/*>*/}
                    {/*    {stream ? (*/}
                    {/*        <video*/}
                    {/*            ref={videoRef}*/}
                    {/*            autoPlay*/}
                    {/*            playsInline*/}
                    {/*            muted={false}*/}
                    {/*            style={{ width: '100%', height: '100%', objectFit: 'cover' }}*/}
                    {/*        />*/}
                    {/*    ) : (*/}
                    {/*        /* Placeholder when no stream yet */}
                    {/*        <Flex*/}
                    {/*            align="center"*/}
                    {/*            justify="center"*/}
                    {/*            width="100%"*/}
                    {/*            height="100%"*/}
                    {/*            direction="column"*/}
                    {/*            gap="2"*/}
                    {/*            style={{ color: 'var(--gray-8)' }}*/}
                    {/*        >*/}
                    {/*            <ChatBubbleIcon width={40} height={40} />*/}
                    {/*            <Text size="3" color="gray">Waiting for stream…</Text>*/}
                    {/*        </Flex>*/}
                    {/*    )}*/}

                    {/*    /!* LIVE badge overlay *!/*/}
                    {/*    {isLive && (*/}
                    {/*        <Box*/}
                    {/*            style={{*/}
                    {/*                position: 'absolute',*/}
                    {/*                top: 12,*/}
                    {/*                left: 12,*/}
                    {/*            }}*/}
                    {/*        >*/}
                    {/*            <Badge color="red" size="2" radius="full">● LIVE</Badge>*/}
                    {/*        </Box>*/}
                    {/*    )}*/}

                    {/*    /!* Viewer count overlay *!/*/}
                    {/*    <Box*/}
                    {/*        style={{*/}
                    {/*            position: 'absolute',*/}
                    {/*            top: 12,*/}
                    {/*            right: 12,*/}
                    {/*            background: 'rgba(0,0,0,0.55)',*/}
                    {/*            borderRadius: 'var(--radius-2)',*/}
                    {/*            padding: '4px 10px',*/}
                    {/*        }}*/}
                    {/*    >*/}
                    {/*        <Flex align="center" gap="1">*/}
                    {/*            <PersonIcon color="white" />*/}
                    {/*            <Text size="2" style={{ color: 'white' }}>{viewerCount.toLocaleString()}</Text>*/}
                    {/*        </Flex>*/}
                    {/*    </Box>*/}
                    {/*</Box>*/}
                    <Box
                        style={{
                            width: '100%',
                            aspectRatio: '16/9',
                            background: 'var(--gray-3)',
                            borderRadius: 'var(--radius-3)',
                            overflow: 'hidden',
                            position: 'relative',
                        }}
                    >
                        <video
                            ref={videoRef}
                            autoPlay
                            playsInline
                            muted={false}
                            style={{
                                width: '100%',
                                height: '100%',
                                objectFit: 'cover',
                                display: isLive ? 'block' : 'none',
                            }}
                        />

                        {/* Placeholder when not connected yet */}
                        {!isLive && (
                            <Flex
                                align="center"
                                justify="center"
                                width="100%"
                                height="100%"
                                direction="column"
                                gap="2"
                                style={{color: 'var(--gray-8)', position: 'absolute', top: 0}}
                            >
                                <ChatBubbleIcon width={40} height={40}/>
                                <Text size="3" color="gray">
                                    {error || 'Connecting to stream…'}
                                </Text>
                            </Flex>
                        )}

                        {/* LIVE badge */}
                        {isLive && (
                            <Box style={{position: 'absolute', top: 12, left: 12}}>
                                <Badge color="red" size="2" radius="full">● LIVE</Badge>
                            </Box>
                        )}

                        {/* Viewer count */}
                        <Box
                            style={{
                                position: 'absolute',
                                top: 12,
                                right: 12,
                                background: 'rgba(0,0,0,0.55)',
                                borderRadius: 'var(--radius-2)',
                                padding: '4px 10px',
                            }}
                        >
                            <Flex align="center" gap="1">
                                <PersonIcon color="white"/>
                                <Text size="2" style={{color: 'white'}}>{viewerCount.toLocaleString()}</Text>
                            </Flex>
                        </Box>
                    </Box>

                    {/* Streamer info row */}
                    <Flex align="center" justify="between" width="100%">
                        <Flex align="center" gap="3">
                            <Avatar
                                size="4"
                                fallback="S"
                                radius="full"
                                style={{background: '#8480c6'}}
                            />
                            <Flex direction="column" gap="1">
                                <Heading size="4">StreamerName</Heading>
                                <Text size="2" color="gray">Just Coding · Building a live streaming platform</Text>
                            </Flex>
                        </Flex>

                        <Flex gap="2">
                            <Button
                                variant={isFollowing ? 'soft' : 'solid'}
                                onClick={() => setIsFollowing(f => !f)}
                            >
                                <HeartIcon/>
                                {isFollowing ? 'Following' : 'Follow'}
                            </Button>
                            <Button variant="outline">Subscribe</Button>
                        </Flex>
                    </Flex>

                    <Separator size="4"/>

                    {/* Stream description */}
                    <Text size="2" color="gray">
                        Building a real-time live streaming platform from scratch in C++ and React. No libs, no cap.
                    </Text>
                </Flex>

                {/* RIGHT — Chat panel — mirrors InteractionHalf */}
                <Flex
                    direction="column"
                    id="InteractionHalf"
                    gap="3"
                    style={{
                        width: '360px',
                        minWidth: '320px',
                        flexShrink: 0,
                        height: '100%',
                    }}
                >
                    <Flex align="center" justify="between">
                        <Heading size="4">Stream Chat</Heading>
                        <Badge color="green" size="1" radius="full">
                            {viewerCount} watching
                        </Badge>
                    </Flex>

                    {/* Chat messages */}
                    <Box
                        style={{
                            background: 'var(--gray-2)',
                            borderRadius: 'var(--radius-3)',
                            flex: 1,
                            minHeight: '420px',
                            maxHeight: '520px',
                            overflow: 'hidden',
                        }}
                    >
                        <ScrollArea style={{height: '100%', padding: '12px'}}>
                            <Flex direction="column" gap="2" p="2">
                                {messages.map(msg => (
                                    <Flex key={msg.id} gap="2" align="start">
                                        <Text
                                            size="2"
                                            style={{color: usernameColor(msg.username), fontWeight: 600, whiteSpace: 'nowrap'}}
                                        >
                                            {msg.username}:
                                        </Text>
                                        <Text size="2" style={{wordBreak: 'break-word'}}>
                                            {msg.text}
                                        </Text>
                                    </Flex>
                                ))}
                                <div ref={chatBottomRef}/>
                            </Flex>
                        </ScrollArea>
                    </Box>

                    {/* Chat input */}
                    <Flex gap="2">
                        <TextField.Root
                            style={{flex: 1}}
                            placeholder="Say something…"
                            value={inputValue}
                            onChange={e => setInputValue(e.target.value)}
                            onKeyDown={e => {
                                if (e.key === 'Enter') handleSendMessage();
                            }}
                        />
                        <IconButton onClick={handleSendMessage} disabled={!inputValue.trim()}>
                            <PaperPlaneIcon/>
                        </IconButton>
                    </Flex>
                </Flex>

            </Flex>
        </Flex>
    );
};

export default StreamView;