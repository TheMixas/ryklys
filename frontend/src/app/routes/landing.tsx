import {Box, Button, Flex, Heading, Text, Badge} from "@radix-ui/themes";
import sharkGif from "../../assets/spinning_ryklys.gif";
import {useNavigate} from "react-router";
import {paths} from "@/config/paths";
import {Navbar} from "@/components/ui/navbar/navbar";
import {RocketIcon, VideoIcon, LightningBoltIcon, GlobeIcon} from "@radix-ui/react-icons";

const FeatureCard = ({icon, title, description}: {
    icon: React.ReactNode;
    title: string;
    description: string;
}) => (
    <Flex
        direction="column"
        align="center"
        gap="2"
        p="5"
        style={{
            background: 'var(--gray-2)',
            borderRadius: 'var(--radius-3)',
            textAlign: 'center',
            flex: '1 1 0',
            minWidth: 200,
            transition: 'transform 0.15s ease, background 0.15s ease',
            cursor: 'default',
        }}
        onMouseEnter={e => {
            (e.currentTarget as HTMLElement).style.transform = 'translateY(-4px)';
            (e.currentTarget as HTMLElement).style.background = 'var(--gray-3)';
        }}
        onMouseLeave={e => {
            (e.currentTarget as HTMLElement).style.transform = 'translateY(0)';
            (e.currentTarget as HTMLElement).style.background = 'var(--gray-2)';
        }}
    >
        <Box style={{
            width: 48,
            height: 48,
            borderRadius: 'var(--radius-3)',
            background: 'var(--accent-3)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
        }}>
            {icon}
        </Box>
        <Text size="3" weight="bold">{title}</Text>
        <Text size="2" color="gray">{description}</Text>
    </Flex>
);

const LandingRoute = () => {
    const navigate = useNavigate();

    return (
        <Flex direction="column" style={{minHeight: "100vh"}}>
            <Navbar/>

            {/* Hero */}
            <Flex
                align="center"
                justify="center"
                direction="column"
                gap="4"
                style={{flex: 1, paddingTop: 40, paddingBottom: 60}}
            >
                {/* Shark + badge */}
                <Flex direction="column" align="center" gap="0" style={{position: 'relative'}}>
                    <Box>
                        <img
                            src={sharkGif}
                            alt="Spinning Shark"
                            style={{
                                width: "240px",
                                height: "auto",
                                filter: "drop-shadow(0 8px 24px rgba(132, 128, 198, 0.3))",
                            }}
                        />
                    </Box>
                    <Badge
                        color="iris"
                        size="2"
                        radius="full"
                        style={{marginTop: -60, zIndex: 1}}
                    >
                        Open Source Streaming
                    </Badge>
                </Flex>

                {/* Headlines */}
                <Flex direction="column" align="center" gap="1" mt="3">
                    <Heading size="9" align="center">
                        Stream sharp,
                    </Heading>
                    <Heading size="9" align="center">
                        stream <span style={{color: 'var(--accent-9)'}}>NOW</span>!
                    </Heading>
                </Flex>

                {/* Subtext */}
                <Flex direction="column" align="center" gap="1" style={{maxWidth: 480}}>
                    <Text size="3" color="gray" align="center">
                        Go live directly from your browser — no downloads, no OBS.
                    </Text>
                    <Text size="3" color="gray" align="center">
                        Just pick a scene, plug in your mic, and you're live.
                    </Text>
                </Flex>

                {/* CTAs */}
                <Flex gap="3" mt="2">
                    <Button
                        size="3"
                        onClick={() => navigate(paths.app.streamerDashboard.getHref())}
                    >
                        <RocketIcon/>
                        Start Broadcasting
                    </Button>
                    <Button
                        size="3"
                        variant="outline"
                        onClick={() => navigate('app/streams')}
                    >
                        <VideoIcon/>
                        Browse Streams
                    </Button>
                </Flex>
            </Flex>

            {/* Features */}
            <Box px="6" pb="9">
                <Flex direction="column" align="center" gap="4" mb="5">
                    <Heading size="5">Why Ryklys?</Heading>
                </Flex>
                <Flex gap="4" wrap="wrap" justify="center" style={{maxWidth: 900, margin: '0 auto'}}>
                    <FeatureCard
                        icon={<LightningBoltIcon width={22} height={22} color="var(--accent-9)"/>}
                        title="Zero Setup"
                        description="No software to install. Open your browser, hit go live, and you're streaming."
                    />
                    <FeatureCard
                        icon={<VideoIcon width={22} height={22} color="var(--accent-9)"/>}
                        title="WebGL Scenes"
                        description="Compose your stream with video captures, overlays, and popups — all in-browser."
                    />
                    <FeatureCard
                        icon={<GlobeIcon width={22} height={22} color="var(--accent-9)"/>}
                        title="Open Source"
                        description="Built from scratch with C++ and React. No black boxes, own every line."
                    />
                </Flex>
            </Box>
        </Flex>
    );
};

export default LandingRoute;