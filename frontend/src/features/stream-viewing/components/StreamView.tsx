import {
    Heading, Text, Flex, Button, Badge, Avatar, Box, Separator
} from '@radix-ui/themes';
import {useState, useRef, useEffect} from 'react';
import {HeartIcon, PersonIcon, ChatBubbleIcon} from '@radix-ui/react-icons';
import Hls from 'hls.js';
import {env} from "@/config/env.ts";
import {useParams} from "react-router";
import ChatPanel, {type ChatMessage} from "@/components/ChatPanel.tsx"; // adjust import path

const StreamView = () => {
    const videoRef = useRef<HTMLVideoElement>(null);
    const hlsRef = useRef<Hls | null>(null);
    const wsRef = useRef<WebSocket | null>(null);
    const [messages, setMessages] = useState<ChatMessage[]>([]);
    const [isFollowing, setIsFollowing] = useState(false);
    const [viewerCount, setViewerCount] = useState(0);
    const [isLive, setIsLive] = useState(false);
    const [error, setError] = useState<string | null>(null);

    const {username} = useParams();

    // Fetch chat history
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
        if (Hls.isSupported()) {
            // const hls = new Hls({
            //     liveDurationInfinity: true,
            //     lowLatencyMode: true,
            //     backBufferLength: 30,
            // });
            const hls = new Hls({
                liveDurationInfinity: true,
                lowLatencyMode: true,
                backBufferLength: 5,              // 5s behind
                liveSyncDurationCount: 1,         // NEW — start playback 1 segment behind live edge
                liveMaxLatencyDurationCount: 3,   // NEW — max 3 segments behind before jumping forward
                maxBufferLength: 3,               // NEW — only buffer 3s ahead
                maxMaxBufferLength: 5,            // NEW — hard cap on buffer
                highBufferWatchdogPeriod: 1,      // NEW — check buffer every 1s
            });

            hls.loadSource(streamUrl);
            hls.attachMedia(video);

            hls.on(Hls.Events.MANIFEST_PARSED, () => {
                setIsLive(true);
                setError(null);
                video.play().catch(() => {});
            });

            hls.on(Hls.Events.ERROR, (_event, data) => {
                if (data.fatal) {
                    if (data.type === Hls.ErrorTypes.NETWORK_ERROR) {
                        setError('Stream not available yet');
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
            video.src = streamUrl;
            video.addEventListener('loadedmetadata', () => {
                setIsLive(true);
                video.play().catch(() => {});
            });
        }
    }, [username]);

    // Viewer WebSocket
    useEffect(() => {
        if (!username) return;

        const wsUrl = `${env.API_URL.replace(/^http/, 'ws')}/ws/view?streamId=${username}`;
        const ws = new WebSocket(wsUrl);
        wsRef.current = ws;

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
                            return [...prev, {
                                id: data.id,
                                username: data.username,
                                text: data.text,
                                createdAt: data.createdAt
                            }];
                        });
                        break;
                }
            } catch (e) {
                console.error('[WS] parse error:', e);
            }
        };

        ws.onclose = (event) => {
            wsRef.current = null;
            if (event.code === 1001 && event.reason === 'Stream ended') {
                setIsLive(false);
                setError('Stream has ended');
            }
        };

        return () => ws.close();
    }, [username]);

    return (
        <Flex direction="column" gap="4" p="6" width="100%">

            <Heading size="8">Stream</Heading>
            <Text color="gray" size="4">
                Watch live streams from your favourite creators.
            </Text>

            <Flex direction="row" gap="6" align="start" justify="between" width="100%">

                {/* LEFT — Video + streamer info */}
                <Flex direction="column" gap="4" width="100%" style={{flex: '1 1 0'}}>

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

                        {isLive && (
                            <Box style={{position: 'absolute', top: 12, left: 12}}>
                                <Badge color="red" size="2" radius="full">● LIVE</Badge>
                            </Box>
                        )}

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

                    {/* Streamer info */}
                    <Flex align="center" justify="between" width="100%">
                        <Flex align="center" gap="3">
                            <Avatar
                                size="4"
                                fallback={username?.[0]?.toUpperCase() ?? 'S'}
                                radius="full"
                                style={{background: '#8480c6'}}
                            />
                            <Flex direction="column" gap="1">
                                <Heading size="4">{username ?? 'StreamerName'}</Heading>
                                <Text size="2" color="gray">Live now</Text>
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
                        </Flex>
                    </Flex>

                    <Separator size="4"/>
                </Flex>

                {/* RIGHT — Chat */}
                <Flex
                    direction="column"
                    style={{
                        width: '360px',
                        minWidth: '320px',
                        flexShrink: 0,
                        height: '100%',
                    }}
                >
                    <ChatPanel
                        messages={messages}
                        viewerCount={viewerCount}
                        wsRef={wsRef}
                    />
                </Flex>
            </Flex>
        </Flex>
    );
};

export default StreamView;