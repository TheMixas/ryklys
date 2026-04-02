import {
    Heading, Text, Flex, Box, Badge, Avatar, Button, Separator, Skeleton
} from '@radix-ui/themes';
import {useState, useEffect} from 'react';
import {useNavigate} from 'react-router';
import {
    PersonIcon, CountdownTimerIcon, VideoIcon,
    BarChartIcon, RocketIcon, ClockIcon
} from '@radix-ui/react-icons';
import {env} from "@/config/env.ts";
import {useAuth} from "@/hooks/useAuth.tsx";

interface StreamHistory {
    streamId: string;
    title: string;
    status: string;
    startedAt: string;
    endedAt: string;
}

interface Stats {
    totalStreams: number;
    liveNow: boolean;
    totalMinutes: number;
}

const AVATAR_COLORS = ['#e05c5c', '#5c9ce0', '#5ce09a', '#c45ce0', '#e0a85c', '#5ce0d4', '#8480c6'];
const getAvatarColor = (id: number) => AVATAR_COLORS[id % AVATAR_COLORS.length];

const formatDuration = (startedAt: string, endedAt: string): string => {
    const start = new Date(startedAt);
    const end = endedAt ? new Date(endedAt) : new Date();
    const diffMs = end.getTime() - start.getTime();
    const mins = Math.floor(diffMs / 60000);
    if (mins < 1) return '<1 min';
    if (mins < 60) return `${mins}m`;
    const hrs = Math.floor(mins / 60);
    return `${hrs}h ${mins % 60}m`;
};

const formatDate = (dateStr: string): string => {
    const d = new Date(dateStr);
    return d.toLocaleDateString('en-GB', {day: 'numeric', month: 'short', year: 'numeric'});
};

const formatTime = (dateStr: string): string => {
    const d = new Date(dateStr);
    return d.toLocaleTimeString('en-GB', {hour: '2-digit', minute: '2-digit'});
};

const computeStats = (streams: StreamHistory[]): Stats => {
    let totalMinutes = 0;
    let liveNow = false;

    for (const s of streams) {
        const start = new Date(s.startedAt);
        const end = s.endedAt ? new Date(s.endedAt) : new Date();
        totalMinutes += (end.getTime() - start.getTime()) / 60000;
        if (s.status === 'live') liveNow = true;
    }

    return {
        totalStreams: streams.length,
        liveNow,
        totalMinutes: Math.floor(totalMinutes),
    };
};

const StatCard = ({icon, label, value}: { icon: React.ReactNode; label: string; value: string }) => (
    <Flex
        direction="column"
        align="center"
        gap="1"
        p="4"
        style={{
            background: 'var(--gray-2)',
            borderRadius: 'var(--radius-3)',
            flex: '1 1 0',
            minWidth: 140,
        }}
    >
        <Box style={{
            width: 40,
            height: 40,
            borderRadius: 'var(--radius-3)',
            background: 'var(--accent-3)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
        }}>
            {icon}
        </Box>
        <Text size="5" weight="bold">{value}</Text>
        <Text size="1" color="gray">{label}</Text>
    </Flex>
);

const StreamHistoryRow = ({stream}: { stream: StreamHistory }) => {
    const isLive = stream.status === 'live';

    return (
        <Flex
            align="center"
            justify="between"
            p="3"
            style={{
                background: 'var(--gray-2)',
                borderRadius: 'var(--radius-2)',
            }}
        >
            <Flex align="center" gap="3" style={{minWidth: 0, flex: 1}}>
                <Box style={{
                    width: 36,
                    height: 36,
                    borderRadius: 'var(--radius-2)',
                    background: isLive ? 'var(--red-3)' : 'var(--gray-3)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    flexShrink: 0,
                }}>
                    <VideoIcon
                        width={16}
                        height={16}
                        color={isLive ? 'var(--red-11)' : 'var(--gray-11)'}
                    />
                </Box>
                <Flex direction="column" gap="0" style={{minWidth: 0}}>
                    <Text size="2" weight="bold" style={{
                        overflow: 'hidden',
                        textOverflow: 'ellipsis',
                        whiteSpace: 'nowrap',
                    }}>
                        {stream.title || 'Untitled stream'}
                    </Text>
                    <Flex align="center" gap="2">
                        <Text size="1" color="gray">{formatDate(stream.startedAt)}</Text>
                        <Text size="1" color="gray">at {formatTime(stream.startedAt)}</Text>
                    </Flex>
                </Flex>
            </Flex>

            <Flex align="center" gap="3" style={{flexShrink: 0}}>
                <Flex align="center" gap="1">
                    <ClockIcon width={12} height={12} color="var(--gray-9)" />
                    <Text size="1" color="gray">
                        {formatDuration(stream.startedAt, stream.endedAt)}
                    </Text>
                </Flex>
                {isLive ? (
                    <Badge color="red" size="1" radius="full">● LIVE</Badge>
                ) : (
                    <Badge color="gray" size="1" radius="full">Ended</Badge>
                )}
            </Flex>
        </Flex>
    );
};

const SkeletonRow = () => (
    <Flex align="center" gap="3" p="3" style={{background: 'var(--gray-2)', borderRadius: 'var(--radius-2)'}}>
        <Skeleton style={{width: 36, height: 36, borderRadius: 'var(--radius-2)'}} />
        <Flex direction="column" gap="1" style={{flex: 1}}>
            <Skeleton style={{width: '60%', height: 14}} />
            <Skeleton style={{width: '30%', height: 10}} />
        </Flex>
    </Flex>
);

const MePage = () => {
    const navigate = useNavigate();
    const {authenticated, user} = useAuth();
    const [streams, setStreams] = useState<StreamHistory[]>([]);
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        if (!authenticated) return;

        const fetchHistory = async () => {
            try {
                const res = await fetch(`${env.API_URL}/api/users/me/streams`, {
                    credentials: 'include',
                });
                if (res.ok) {
                    const data = await res.json();
                    setStreams(data);
                }
            } catch (e) {
                console.error('[Me] Failed to fetch stream history:', e);
            } finally {
                setLoading(false);
            }
        };

        fetchHistory();
    }, [authenticated]);

    // Not logged in
    if (!authenticated) {
        return (
            <Flex
                align="center"
                justify="center"
                direction="column"
                gap="3"
                style={{minHeight: '60vh'}}
            >
                <PersonIcon width={48} height={48} color="var(--gray-8)" />
                <Heading size="5" color="gray">You're not logged in</Heading>
                <Text size="2" color="gray">Log in to see your profile and stream history.</Text>
                <Button mt="2" onClick={() => navigate('/login')}>
                    Log in
                </Button>
            </Flex>
        );
    }

    const stats = computeStats(streams);
    const totalHours = Math.floor(stats.totalMinutes / 60);
    const remainingMins = stats.totalMinutes % 60;

    return (
        <Flex direction="column" gap="5" p="6" width="100%" style={{maxWidth: 800, margin: '0 auto'}}>

            {/* Profile header */}
            <Flex align="center" gap="4">
                <Avatar
                    size="6"
                    fallback={user?.username?.[0]?.toUpperCase() ?? '?'}
                    radius="full"
                    style={{background: getAvatarColor(user?.id ?? 0)}}
                />
                <Flex direction="column" gap="1">
                    <Flex align="center" gap="2">
                        <Heading size="7">{user?.username}</Heading>
                        {stats.liveNow && (
                            <Badge color="red" size="1" radius="full">● LIVE NOW</Badge>
                        )}
                    </Flex>
                    <Text size="2" color="gray">User #{user?.id}</Text>
                </Flex>
            </Flex>

            <Separator size="4" />

            {/* Stats row */}
            <Flex gap="3" wrap="wrap">
                <StatCard
                    icon={<VideoIcon width={18} height={18} color="var(--accent-9)" />}
                    label="Total Streams"
                    value={String(stats.totalStreams)}
                />
                <StatCard
                    icon={<CountdownTimerIcon width={18} height={18} color="var(--accent-9)" />}
                    label="Time Streamed"
                    value={totalHours > 0 ? `${totalHours}h ${remainingMins}m` : `${remainingMins}m`}
                />
                <StatCard
                    icon={<BarChartIcon width={18} height={18} color="var(--accent-9)" />}
                    label="Status"
                    value={stats.liveNow ? 'Live' : 'Offline'}
                />
            </Flex>

            <Separator size="4" />

            {/* Stream history */}
            <Flex direction="column" gap="3">
                <Flex align="center" justify="between">
                    <Heading size="5">Stream History</Heading>
                    <Button
                        variant="soft"
                        size="1"
                        onClick={() => navigate('/app/s-dashboard')}
                    >
                        <RocketIcon />
                        Go Live
                    </Button>
                </Flex>

                {loading ? (
                    <Flex direction="column" gap="2">
                        {[1, 2, 3, 4].map(i => <SkeletonRow key={i} />)}
                    </Flex>
                ) : streams.length > 0 ? (
                    <Flex direction="column" gap="2">
                        {streams.map(stream => (
                            <StreamHistoryRow key={stream.streamId} stream={stream} />
                        ))}
                    </Flex>
                ) : (
                    <Flex
                        direction="column"
                        align="center"
                        justify="center"
                        gap="2"
                        py="7"
                        style={{
                            background: 'var(--gray-2)',
                            borderRadius: 'var(--radius-3)',
                        }}
                    >
                        <VideoIcon width={36} height={36} color="var(--gray-8)" />
                        <Heading size="3" color="gray">No streams yet</Heading>
                        <Text size="2" color="gray">Go live for the first time!</Text>
                        <Button
                            mt="2"
                            size="2"
                            onClick={() => navigate('/app/s-dashboard')}
                        >
                            <RocketIcon />
                            Start Streaming
                        </Button>
                    </Flex>
                )}
            </Flex>
        </Flex>
    );
};

export default MePage;
