import {
    Heading, Text, Flex, Box, Badge, Avatar, Card,
    Button, Separator, Skeleton
} from '@radix-ui/themes';
import {useState, useEffect} from 'react';
import {useNavigate} from 'react-router';
import {
    PlayIcon, PersonIcon, CountdownTimerIcon,
    VideoIcon, RocketIcon
} from '@radix-ui/react-icons';
import {env} from "@/config/env.ts";
import {paths} from "@/config/paths.ts";

interface LiveStream {
    streamId: string;
    userId: number;
    title: string;
    startedAt: string;
    username: string;
    viewerCount?: number;
    thumbnailUrl?: string;
}

// Placeholder colors for streamer avatars
const AVATAR_COLORS = ['#e05c5c', '#5c9ce0', '#5ce09a', '#c45ce0', '#e0a85c', '#5ce0d4', '#8480c6'];
const getAvatarColor = (id: number) => AVATAR_COLORS[id % AVATAR_COLORS.length];

// Time ago helper
const timeAgo = (dateStr: string): string => {
    const now = new Date();
    const then = new Date(dateStr);
    const diffMs = now.getTime() - then.getTime();
    const mins = Math.floor(diffMs / 60000);
    if (mins < 1) return 'Just started';
    if (mins < 60) return `${mins}m ago`;
    const hrs = Math.floor(mins / 60);
    if (hrs < 24) return `${hrs}h ${mins % 60}m`;
    return `${Math.floor(hrs / 24)}d ago`;
};

const StreamCard = ({stream, onClick}: { stream: LiveStream; onClick: () => void }) => (

    <Box
        onClick={onClick}
        style={{
            cursor: 'pointer',
            borderRadius: 'var(--radius-3)',
            overflow: 'hidden',
            transition: 'transform 0.15s ease, box-shadow 0.15s ease',
        }}
        onMouseEnter={e => {
            (e.currentTarget as HTMLElement).style.transform = 'translateY(-2px)';
            (e.currentTarget as HTMLElement).style.boxShadow = '0 8px 24px rgba(0,0,0,0.12)';
        }}
        onMouseLeave={e => {
            (e.currentTarget as HTMLElement).style.transform = 'translateY(0)';
            (e.currentTarget as HTMLElement).style.boxShadow = 'none';
        }}
    >
        {/* Thumbnail / Placeholder */}
        <Box
            style={{
                width: '100%',
                aspectRatio: '16/9',
                background: `linear-gradient(135deg, ${getAvatarColor(stream.userId)}, var(--gray-4))`,
                position: 'relative',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
            }}
        >
            <VideoIcon width={40} height={40} color="white" style={{opacity: 0.4}}/>

            {/* LIVE badge */}
            <Box style={{position: 'absolute', top: 8, left: 8}}>
                <Badge color="red" size="1" radius="full">● LIVE</Badge>
            </Box>

            {/* Viewer count */}
            <Box
                style={{
                    position: 'absolute',
                    bottom: 8,
                    right: 8,
                    background: 'rgba(0,0,0,0.6)',
                    borderRadius: 'var(--radius-2)',
                    padding: '2px 8px',
                }}
            >
                <Flex align="center" gap="1">
                    <PersonIcon color="white" width={12} height={12}/>
                    <Text size="1" style={{color: 'white'}}>
                        {stream.viewerCount ?? Math.floor(Math.random() * 200 + 5)}
                    </Text>
                </Flex>
            </Box>

            {/* Started time */}
            <Box
                style={{
                    position: 'absolute',
                    bottom: 8,
                    left: 8,
                    background: 'rgba(0,0,0,0.6)',
                    borderRadius: 'var(--radius-2)',
                    padding: '2px 8px',
                }}
            >
                <Flex align="center" gap="1">
                    <CountdownTimerIcon color="white" width={12} height={12}/>
                    <Text size="1" style={{color: 'white'}}>{timeAgo(stream.startedAt)}</Text>
                </Flex>
            </Box>
        </Box>

        {/* Stream info */}
        <Box p="3" style={{background: 'var(--gray-2)'}}>
            <Flex gap="3" align="start">
                <Avatar
                    size="2"
                    fallback={stream.username?.[0]?.toUpperCase() ?? 'S'}
                    radius="full"
                    style={{background: getAvatarColor(stream.userId), flexShrink: 0}}
                />
                <Flex direction="column" gap="1" style={{minWidth: 0}}>
                    <Text size="2" weight="bold" style={{
                        overflow: 'hidden',
                        textOverflow: 'ellipsis',
                        whiteSpace: 'nowrap'
                    }}>
                        {stream.title}
                    </Text>
                    <Text size="1" color="gray">
                        {stream.username ?? `User #${stream.userId}`}
                    </Text>
                </Flex>
            </Flex>
        </Box>
    </Box>
);

const StreamCardSkeleton = () => (
    <Box style={{borderRadius: 'var(--radius-3)', overflow: 'hidden'}}>
        <Skeleton style={{width: '100%', aspectRatio: '16/9'}}/>
        <Box p="3" style={{background: 'var(--gray-2)'}}>
            <Flex gap="3" align="start">
                <Skeleton style={{width: 32, height: 32, borderRadius: '50%'}}/>
                <Flex direction="column" gap="1" style={{flex: 1}}>
                    <Skeleton style={{width: '80%', height: 16}}/>
                    <Skeleton style={{width: '40%', height: 12}}/>
                </Flex>
            </Flex>
        </Box>
    </Box>
);

const TopStreamerCard = ({username, userId, isLive}: {
    username: string;
    userId: number;
    isLive: boolean
}) => (
    <Flex
        align="center"
        gap="3"
        p="3"
        style={{
            background: 'var(--gray-2)',
            borderRadius: 'var(--radius-3)',
            cursor: 'pointer',
            transition: 'background 0.15s ease',
        }}
        onMouseEnter={e => {
            (e.currentTarget as HTMLElement).style.background = 'var(--gray-3)';
        }}
        onMouseLeave={e => {
            (e.currentTarget as HTMLElement).style.background = 'var(--gray-2)';
        }}
    >
        <Box style={{position: 'relative'}}>
            <Avatar
                size="3"
                fallback={username[0].toUpperCase()}
                radius="full"
                style={{background: getAvatarColor(userId)}}
            />
            {isLive && (
                <Box
                    style={{
                        position: 'absolute',
                        bottom: -1,
                        right: -1,
                        width: 12,
                        height: 12,
                        borderRadius: '50%',
                        background: '#e5484d',
                        border: '2px solid var(--gray-2)',
                    }}
                />
            )}
        </Box>
        <Flex direction="column" gap="0">
            <Text size="2" weight="bold">{username}</Text>
            <Text size="1" color="gray">{isLive ? 'Live now' : 'Offline'}</Text>
        </Flex>
    </Flex>
);

const BrowseStreamsPage = () => {
    const navigate = useNavigate();
    const [streams, setStreams] = useState<LiveStream[]>([]);
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        const fetchStreams = async () => {
            try {
                const res = await fetch(`${env.API_URL}/api/streams/live`);
                const data = await res.json();
                console.log("Fetched live streams data.json: " + data)
                setStreams(data);
            } catch (err) {
                console.error('Failed to fetch streams:', err);
            } finally {
                setLoading(false);
            }
        };

        fetchStreams();
        const interval = setInterval(fetchStreams, 10000); // Refresh every 10s
        return () => clearInterval(interval);
    }, []);

    const handleWatchStream = (streamUsername: string) => {
        navigate(`/app/viewStream/${streamUsername}`);
    };

    return (
        <Flex direction="column" gap="4" p="6" width="100%">

            {/* Page header */}
            <Flex align="center" justify="between">
                <Flex direction="column" gap="1">
                    <Heading size="8">Browse Streams</Heading>
                    <Text color="gray" size="4">
                        Watch live streams from the community.
                    </Text>
                </Flex>
                <Flex align="center" gap="2">
                    <Box
                        style={{
                            width: 8,
                            height: 8,
                            borderRadius: '50%',
                            background: streams.length > 0 ? '#e5484d' : 'var(--gray-8)',
                            animation: streams.length > 0 ? 'pulse 2s infinite' : 'none',
                        }}
                    />
                    <Text size="2" color="gray">
                        {streams.length} stream{streams.length !== 1 ? 's' : ''} live
                    </Text>
                </Flex>
            </Flex>

            {/* Live streams grid */}
            {loading ? (

                <Box style={{
                    display: 'grid',
                    gridTemplateColumns: 'repeat(auto-fill, minmax(300px, 1fr))',
                    gap: 'var(--space-4)',
                }}>
                    {[1, 2, 3, 4, 5, 6].map(i => <StreamCardSkeleton key={i}/>)}
                </Box>
            ) : streams.length > 0 ? (
                <Box style={{
                    display: 'grid',
                    gridTemplateColumns: 'repeat(auto-fill, minmax(300px, 1fr))',
                    gap: 'var(--space-4)',
                }}>
                    {streams.map(stream => (
                        <StreamCard
                            key={stream.streamId}
                            stream={stream}
                            onClick={() => handleWatchStream(stream.username)}
                        />
                    ))}
                </Box>
            ) : (
                <Flex
                    direction="column"
                    align="center"
                    justify="center"
                    gap="3"
                    py="9"
                    style={{
                        background: 'var(--gray-2)',
                        borderRadius: 'var(--radius-3)',
                    }}
                >
                    <VideoIcon width={48} height={48} color="var(--gray-8)"/>
                    <Heading size="4" color="gray">No one is live right now</Heading>
                    <Text size="2" color="gray">Be the first to start streaming!</Text>
                    <Button
                        mt="2"
                        onClick={() => navigate("/"+paths.app.streamerDashboard.path)}
                    >
                        <RocketIcon/>
                        Start Streaming
                    </Button>
                </Flex>
            )}

            <Separator size="4" my="4"/>

            {/* Top streamers section */}
            <Flex direction="column" gap="3">
                <Heading size="5">Top Streamers</Heading>
                <Text size="2" color="gray">Popular creators in the community</Text>

                <Box style={{
                    display: 'grid',
                    gridTemplateColumns: 'repeat(auto-fill, minmax(220px, 1fr))',
                    gap: 'var(--space-3)',
                }}>
                    <TopStreamerCard username="alice" userId={1} isLive={streams.some(s => s.userId === 1)}/>
                    <TopStreamerCard username="bob" userId={2} isLive={streams.some(s => s.userId === 2)}/>
                    <TopStreamerCard username="charlie" userId={3} isLive={streams.some(s => s.userId === 3)}/>
                </Box>
            </Flex>

            {/* Pulse animation */}
            <style>{`
                @keyframes pulse {
                    0%, 100% { opacity: 1; }
                    50% { opacity: 0.4; }
                }
            `}</style>
        </Flex>
    );
};

export default BrowseStreamsPage;
