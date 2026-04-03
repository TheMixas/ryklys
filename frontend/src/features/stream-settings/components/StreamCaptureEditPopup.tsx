import {
    Flex, Button, Text, Heading, Box, Badge, Separator, Dialog
} from '@radix-ui/themes';
import {
    VideoIcon, ImageIcon, SpeakerLoudIcon, CheckIcon
} from '@radix-ui/react-icons';
import {type VisualCapture, VideoCapture} from "@/features/stream/models/VisualCapture.ts";
import type {AudioCapture} from "@/features/stream/models/AudioCapture.ts";

interface StreamCaptureEditPopupProps {
    isOpen: boolean;
    onOpenChange: (open: boolean) => void;
    videoCaptures: VideoCapture[];
    imageCaptures: VisualCapture[];
    audioCaptures: AudioCapture[];
    setVideoElement: (slot: number, videoElement: HTMLVideoElement) => void;
    setImageElement: (slot: number, imageElement: HTMLImageElement) => void;
    setAudioStream: (slot: number, stream: MediaStream) => void;
}

interface SourceRowProps {
    icon: React.ReactNode;
    label: string;
    detail: string;
    hasSource: boolean;
    buttonLabel: [string, string];
    onSelect: () => void;
}

const SourceRow = ({icon, label, detail, hasSource, buttonLabel, onSelect}: SourceRowProps) => (
    <Flex
        align="center"
        justify="between"
        p="3"
        style={{
            background: 'var(--gray-3)',
            borderRadius: 'var(--radius-2)',
        }}
    >
        <Flex align="center" gap="3">
            <Box
                style={{
                    width: 36,
                    height: 36,
                    borderRadius: 'var(--radius-2)',
                    background: hasSource ? 'var(--green-3)' : 'var(--gray-4)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    flexShrink: 0,
                }}
            >
                {hasSource
                    ? <CheckIcon width={16} height={16} color="var(--green-11)"/>
                    : icon
                }
            </Box>
            <Flex direction="column" gap="0">
                <Text size="2" weight="bold">{label}</Text>
                <Text size="1" color="gray">{detail}</Text>
            </Flex>
        </Flex>
        <Flex align="center" gap="2">
            {hasSource && <Badge color="green" size="1" radius="full">Active</Badge>}
            <Button
                size="1"
                variant={hasSource ? 'soft' : 'solid'}
                onClick={onSelect}
            >
                {hasSource ? buttonLabel[1] : buttonLabel[0]}
            </Button>
        </Flex>
    </Flex>
);

const StreamCaptureEditPopup = ({
                                    isOpen,
                                    onOpenChange,
                                    videoCaptures,
                                    audioCaptures,
                                    setVideoElement,
                                    imageCaptures,
                                    setImageElement,
                                    setAudioStream
                                }: StreamCaptureEditPopupProps) => {

    const selectScreen = async (index: number) => {
        const screenStream = await navigator.mediaDevices.getDisplayMedia({
            video: {frameRate: 60},
            audio: true
        });
        const videoEl = document.createElement('video');
        videoEl.srcObject = screenStream;
        videoEl.muted = true;
        videoEl.autoplay = true;
        await new Promise<void>(resolve => {
            videoEl.onloadeddata = () => resolve();
            videoEl.play().catch(console.error);
        });
        setVideoElement(index, videoEl);
    };

    const selectImage = (index: number) => {
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = 'image/*';
        input.onchange = async () => {
            const file = input.files?.[0];
            if (!file) return;
            if (file.size > 5 * 1024 * 1024) {
                alert('Image must be under 5MB');
                return;
            }
            const imgEl = new Image();
            await new Promise<void>((resolve, reject) => {
                imgEl.onload = () => resolve();
                imgEl.onerror = reject;
                imgEl.src = URL.createObjectURL(file);
            });
            setImageElement(index, imgEl);
        };
        input.click();
    };

    const selectAudio = async (index: number) => {
        const audioStream = await navigator.mediaDevices.getUserMedia({
            audio: true,
            video: false
        });
        setAudioStream(index, audioStream);
    };

    return (
        <Dialog.Root open={isOpen} onOpenChange={onOpenChange}>
            <Dialog.Content maxWidth="560px">
                <Dialog.Title>Scene Setup</Dialog.Title>
                <Dialog.Description size="2" color="gray" mb="4">
                    Choose your video sources, images, and audio inputs.
                </Dialog.Description>

                <Separator size="4" mb="4"/>

                {/* Video Sources */}
                <Flex direction="column" gap="3" mb="5">
                    <Flex align="center" gap="2">
                        <VideoIcon width={16} height={16} color="var(--accent-9)"/>
                        <Heading size="3">Video Sources</Heading>
                    </Flex>
                    <Flex direction="column" gap="2">
                        {videoCaptures.map((capture, index) => (
                            <SourceRow
                                key={`video-${index}`}
                                icon={<VideoIcon width={16} height={16} color="var(--gray-9)"/>}
                                label={`Video Slot ${index + 1}`}
                                detail={
                                    capture.position
                                        ? `(${capture.position.x}, ${capture.position.y}) · ${capture.size?.width}×${capture.size?.height}`
                                        : 'No position set'
                                }
                                hasSource={!!capture.getSourceElement()}
                                buttonLabel={['Select', 'Change']}
                                onSelect={() => selectScreen(index)}
                            />
                        ))}
                    </Flex>
                </Flex>

                {/* Image Overlays */}
                {imageCaptures.length > 0 && (
                    <Flex direction="column" gap="3" mb="5">
                        <Flex align="center" gap="2">
                            <ImageIcon width={16} height={16} color="var(--accent-9)"/>
                            <Heading size="3">Image Overlays</Heading>
                        </Flex>
                        <Flex direction="column" gap="2">
                            {imageCaptures.map((capture, index) => (
                                <SourceRow
                                    key={`image-${index}`}
                                    icon={<ImageIcon width={16} height={16} color="var(--gray-9)"/>}
                                    label={`Image Slot ${index + 1}`}
                                    detail={
                                        capture.position
                                            ? `(${capture.position.x}, ${capture.position.y})`
                                            : 'No position set'
                                    }
                                    hasSource={!!capture.getSourceElement()}
                                    buttonLabel={['Upload', 'Change']}
                                    onSelect={() => selectImage(index)}
                                />
                            ))}
                        </Flex>
                    </Flex>
                )}

                {/* Audio Inputs */}
                <Flex direction="column" gap="3" mb="5">
                    <Flex align="center" gap="2">
                        <SpeakerLoudIcon width={16} height={16} color="var(--accent-9)"/>
                        <Heading size="3">Audio Inputs</Heading>
                    </Flex>
                    <Flex direction="column" gap="2">
                        {audioCaptures.map((capture, index) => (
                            <SourceRow
                                key={`audio-${index}`}
                                icon={<SpeakerLoudIcon width={16} height={16} color="var(--gray-9)"/>}
                                label={`Audio Slot ${index + 1}`}
                                detail={capture.getMediaStream() ? 'Microphone connected' : 'No input selected'}
                                hasSource={!!capture.getMediaStream()}
                                buttonLabel={['Select', 'Change']}
                                onSelect={() => selectAudio(index)}
                            />
                        ))}
                    </Flex>
                </Flex>

                <Separator size="4" mb="4"/>

                <Flex justify="end" gap="2">
                    <Dialog.Close>
                        <Button variant="soft" color="gray">Cancel</Button>
                    </Dialog.Close>
                    <Dialog.Close>
                        <Button>
                            <CheckIcon/>
                            Confirm
                        </Button>
                    </Dialog.Close>
                </Flex>
            </Dialog.Content>
        </Dialog.Root>
    );
};

export default StreamCaptureEditPopup;