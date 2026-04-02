import {Heading, Text, Flex, Button} from '@radix-ui/themes';
import {useState} from 'react';
import StreamVideo from "@/features/stream-settings/components/StreamVideo.tsx";
import StreamControlRow from "@/features/stream-settings/components/StreamControlRow.tsx";
import StreamQuickActionGrid from "@/features/stream-settings/components/StreamQuickActionGrid.tsx";
import RecentDashboard from "@/features/recent-actions/components/recent-dashboard.tsx";
import StreamCaptureEditPopup from "@/features/stream-settings/components/StreamCaptureEditPopup.tsx";
import {type SceneConfig} from "@/features/stream/models/Scene.ts";
import {ImageCapture, VideoCapture} from "@/features/stream/models/VisualCapture.ts";
import {useLiveStream} from "@/features/stream/hooks/useLiveStream.tsx";
import {AudioCapture} from "@/features/stream/models/AudioCapture.ts";
import StreamAudioMixer from "@/features/stream-settings/components/AudioStreamMixer.tsx";

const sceneConfig: SceneConfig = {
    resolution: {width: 1920, height: 1080},
    backgroundColor: '#8480c6',
    videoCaptures: [
        new VideoCapture(null, {
                x: 0, y: 360
            }, {
                width: 1920, height: 540
            },
            '#ed5e5e'
            , 0),
        new VideoCapture(null, {
                x: 0, y: 0
            }, {
                width: 640, height: 360
            },
            '#5e9ced'
            , 1)
    ],
    // imageCaptures: [
    //     new ImageCapture(null, {
    //             x: 0, y: 0
    //         }, {
    //             width: 1920, height: 1080
    //         },
    //         '#5ee27c'
    //         , 2)
    // ],
    audioCaptures:[
        new AudioCapture(null, null),
        new AudioCapture(null, null)
    ]
}

const StreamerDashboard = () => {
    const {outputStream, videoCaptures, imageCaptures,audioCaptures, setVideoElement, setImageElement,setAudioStream, goLive, pokeWs}
        = useLiveStream({sceneConfig});

    const [isPopupOpen, setIsPopupOpen] = useState(false);

    const handleEditStreamCapture = () => {
        setIsPopupOpen(true);
    }
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
            console.log("Loaded video file into slot", slot);
        };
        input.click();
    };
    return (
        <Flex direction="column" gap="4" p="6" width={'100%'}>
            <Heading size="8">Streamer Dashboard</Heading>
            <Text color="gray" size="4">
                Welcome to your dashboard! Here you can manage your streams, view analytics, and customize your profile.
            </Text>
            <Flex direction="row" gap="6" align={"center"} justify="between" width={"100%"}>
                <Flex direction="column" id={"ControlHalf"} gap="4" width={"100%"}>
                    <StreamVideo stream={outputStream}/>
                    <Button onClick={() => handleEditStreamCapture()}>Choose what to stream!</Button>
                    <StreamControlRow onPlay={() => {
                        goLive().then((response) => {
                            console.log(response)
                        });
                    }} onPause={() => {
                        pokeWs("Hellouuu")
                    }} onStop={() => {
                    }}/>
                    <button onClick={() => loadVideoFile(0)}>Load Video File (Slot 0)</button>
                    <StreamAudioMixer audioCaptures={audioCaptures} />
                    <StreamQuickActionGrid/>
                </Flex>
                <Flex direction="column" id={"InteractionHalf"} width={"100%"} gap="4">
                    <Flex direction="row" gap="7" justify={"center"} align={"center"}>
                        <RecentDashboard/>
                        <RecentDashboard/>
                    </Flex>

                </Flex>
            </Flex>
            {/* Add more dashboard components and features here */}

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