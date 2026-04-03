import {
    Heading, Text, Flex, Button, Badge, Box, Separator
} from '@radix-ui/themes';
import {useState, useRef, useEffect} from 'react';
import {
    PlayIcon, StopIcon, VideoIcon, GearIcon,
    SpeakerLoudIcon, PersonIcon
} from '@radix-ui/react-icons';
import StreamVideo from "@/features/stream-settings/components/StreamVideo.tsx";
import StreamCaptureEditPopup from "@/features/stream-settings/components/StreamCaptureEditPopup.tsx";
import StreamAudioMixer from "@/features/stream-settings/components/AudioStreamMixer.tsx";
import {type SceneConfig} from "@/features/stream/models/Scene.ts";
import {ImageCapture, VideoCapture} from "@/features/stream/models/VisualCapture.ts";
import {useLiveStream} from "@/features/stream/hooks/useLiveStream.tsx";
import {AudioCapture} from "@/features/stream/models/AudioCapture.ts";
import ChatPanel, {type ChatMessage} from "@/components/ChatPanel.tsx"; // adjust import path
import {env} from "@/config/env.ts";
import {useAuth} from "@/hooks/useAuth.tsx";

// const sceneConfig: SceneConfig = {
//     resolution: {width: 1920, height: 1080},
//     backgroundColor: '#8480c6',
//     videoCaptures: [
//         new VideoCapture(null, {x: 0, y: 360}, {width: 1920, height: 540}, '#ed5e5e', 0),
//         new VideoCapture(null, {x: 0, y: 0}, {width: 640, height: 360}, '#5e9ced', 1)
//     ],
//     audioCaptures: [
//         new AudioCapture(null, null),
//         new AudioCapture(null, null)
//     ]
// };
const sceneConfig: SceneConfig = {
    resolution: {width: 1920, height: 1080},
    backgroundColor: '#000000', // Won't really be seen since the screen is full
    videoCaptures: [
        // Main Screen (Gameplay/Desktop)
        // Fills the exact 1920x1080 resolution, z-index 0 (bottom layer)
        new VideoCapture(null, {x: 0, y: 0}, {width: 1920, height: 1080}, '#2b2b2b', 0),

        // Webcam Capture
        // 16:9 ratio (480x270), tucked perfectly into the bottom right corner, z-index 1 (top layer)
        // new VideoCapture(null, {x: 1440, y: 810}, {width: 480, height: 270}, '#e94560', 1)
    ],
    imageCaptures:[
        new ImageCapture(null, {x: 1440, y: 810}, {width: 480, height: 270}, '#e94560', 1)
    ],
    audioCaptures: [
        new AudioCapture(null, null),
        new AudioCapture(null, null)
    ]
};
const StreamerDashboard = () => {
    const {
        outputStream, videoCaptures, imageCaptures, audioCaptures,
        setVideoElement, setImageElement, setAudioStream, goLive, pokeWs
    } = useLiveStream({sceneConfig});

    const [isPopupOpen, setIsPopupOpen] = useState(false);
    const [isLive, setIsLive] = useState(false);
    const [streamId, setStreamId] = useState<string | null>(null);
    const [viewerCount, setViewerCount] = useState(0);
    const [messages, setMessages] = useState<ChatMessage[]>([]);
    const wsRef = useRef<WebSocket | null>(null);
    const {user} = useAuth();

    // Connect to chat WebSocket once we're live
    useEffect(() => {
        if (!user?.username) return;

        const fetchHistory = async () => {
            try {
                const res = await fetch(
                    `${env.API_URL}/api/streams/${user.username}/chat?limit=50`,
                    {credentials: 'include'}
                );
                if (res.ok) {
                    const data: ChatMessage[] = await res.json();
                    setMessages(data);
                }
            } catch (e) {
                console.error('[Dashboard Chat] Failed to fetch history:', e);
            }
        };
        fetchHistory();

        const wsUrl = `${env.API_URL.replace(/^http/, 'ws')}/ws/view?streamId=${user.username}`;
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
                console.error('[Dashboard WS] parse error:', e);
            }
        };

        ws.onclose = () => {
            wsRef.current = null;
        };

        return () => ws.close();
    }, [user?.username]);
    const handleGoLive = async () => {
        try {
            await goLive();
            setStreamId(user?.username ?? "ERROR");
            setIsLive(true);
        } catch (e) {
            console.error('[Dashboard] Failed to go live:', e);
        }
    };

    const handleStopStream = () => {
        // Close WebSocket, reset state
        if (wsRef.current) wsRef.current.close();
        setIsLive(false);
        setStreamId(null);
        setViewerCount(0);
        setMessages([]);
    };

    const loadVideoFile = (slot: number) => {
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = 'video/*';
        input.onchange = async (e) => {
            const file = (e.target as HTMLInputElement).files?.[0];
            if (!file) return;
            const url = URL.createObjectURL(file);
            const video = document.createElement('video');
            video.src = url;
            video.loop = true;
            video.muted = true;
            video.playsInline = true;
            await video.play();
            setVideoElement(slot, video);
        };
        input.click();
    };

    return (
        <Flex direction="column" gap="4" p="6" width="100%">

            {/* Header */}
            <Flex align="center" justify="between">
                <Flex direction="column" gap="1">
                    <Heading size="8">Streamer Dashboard</Heading>
                    <Text color="gray" size="4">
                        Manage your stream, interact with your audience.
                    </Text>
                </Flex>
                <Flex align="center" gap="3">
                    {isLive && (
                        <>
                            <Badge color="red" size="2" radius="full">● LIVE</Badge>
                            <Flex
                                align="center"
                                gap="1"
                                style={{
                                    background: 'rgba(0,0,0,0.55)',
                                    borderRadius: 'var(--radius-2)',
                                    padding: '4px 10px',
                                }}
                            >
                                <PersonIcon color="white"/>
                                <Text size="2" style={{color: 'white'}}>{viewerCount}</Text>
                            </Flex>
                        </>
                    )}
                </Flex>
            </Flex>

            <Separator size="4"/>

            {/* Two-column layout */}
            <Flex direction="row" gap="6" align="start" justify="between" width="100%">

                {/* LEFT — Preview + Controls */}
                <Flex direction="column" gap="4" style={{flex: '1 1 0'}}>

                    {/* Video preview */}
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
                        <StreamVideo stream={outputStream}/>

                        {isLive && (
                            <Box style={{position: 'absolute', top: 12, left: 12}}>
                                <Badge color="red" size="2" radius="full">● LIVE</Badge>
                            </Box>
                        )}
                    </Box>

                    {/* Stream controls */}
                    <Flex gap="2" wrap="wrap">
                        {!isLive ? (
                            <Button size="3" onClick={handleGoLive}>
                                <PlayIcon/>
                                Go Live
                            </Button>
                        ) : (
                            <Button size="3" color="red" onClick={handleStopStream}>
                                <StopIcon/>
                                End Stream
                            </Button>
                        )}
                        <Button
                            variant="soft"
                            size="3"
                            onClick={() => setIsPopupOpen(true)}
                        >
                            <VideoIcon/>
                            Scene Setup
                        </Button>
                        {/*<Button*/}
                        {/*    variant="soft"*/}
                        {/*    size="3"*/}
                        {/*    onClick={() => loadVideoFile(0)}*/}
                        {/*>*/}
                        {/*    <GearIcon/>*/}
                        {/*    Load Video*/}
                        {/*</Button>*/}
                    </Flex>

                    {/* Audio mixer */}
                    <Flex direction="column" gap="2">
                        <Flex align="center" gap="2">
                            <SpeakerLoudIcon/>
                            <Heading size="3">Audio Mixer</Heading>
                        </Flex>
                        <Box
                            style={{
                                background: 'var(--gray-2)',
                                borderRadius: 'var(--radius-3)',
                                padding: '16px',
                            }}
                        >
                            <StreamAudioMixer audioCaptures={audioCaptures}/>
                        </Box>
                    </Flex>
                </Flex>

                {/* RIGHT — Chat */}
                {/* RIGHT — Chat */}
                <Flex
                    direction="column"
                    style={{
                        width: '360px',
                        minWidth: '320px',
                        flexShrink: 0,
                    }}
                >
                    <ChatPanel
                        messages={messages}
                        viewerCount={viewerCount}
                        wsRef={wsRef}
                        title={isLive ? "Live Chat" : "Chat"}
                    />
                </Flex>
            </Flex>

            {/* Scene edit popup */}
            <StreamCaptureEditPopup
                isOpen={isPopupOpen}
                onOpenChange={setIsPopupOpen}
                videoCaptures={videoCaptures}
                setVideoElement={setVideoElement}
                imageCaptures={imageCaptures}
                setImageElement={setImageElement}
                setAudioStream={setAudioStream}
                audioCaptures={audioCaptures}
            />
        </Flex>
    );
};

export default StreamerDashboard;
