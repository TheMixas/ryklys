import * as Dialog from '@radix-ui/react-dialog';
import {Flex, Button, Text} from '@radix-ui/themes';
import {ChevronDownIcon} from '@radix-ui/react-icons';
import './StreamCaptureEditPopup.css';
import {type VisualCapture, VideoCapture} from "@/features/stream/models/VisualCapture.ts";
import type {AudioCapture} from "@/features/stream/models/AudioCapture.ts";

interface StreamCaptureEditPopupProps {
    isOpen: boolean;
    onOpenChange: (open: boolean) => void;
    videoCaptures: VideoCapture[];
    imageCaptures: VisualCapture[];
    audioCaptures: AudioCapture[];
    setVideoElement: (slot: number, videoElement: HTMLVideoElement) => void
    setImageElement: (slot: number, imageElement: HTMLImageElement) => void
    setAudioStream: (slot: number, stream: MediaStream) => void
}

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

    const handleConfirm = () => {
        onOpenChange(false);
    };

    return (
        <Dialog.Root open={isOpen} onOpenChange={onOpenChange}>
            <Dialog.Portal>
                <Dialog.Overlay className="StreamCaptureEditPopup__overlay"/>
                <Dialog.Content className="StreamCaptureEditPopup__content">
                    <div className="StreamCaptureEditPopup__header">
                        <Dialog.Title className="StreamCaptureEditPopup__title">Stream Capture Settings</Dialog.Title>
                        <Dialog.Close asChild>
                            <button className="StreamCaptureEditPopup__close" aria-label="Close">
                                ✕
                            </button>
                        </Dialog.Close>
                    </div>

                    <Flex direction="column" gap="5" style={{padding: '20px 0'}}>
                        Video Stream Select
                        {
                            videoCaptures.map((capture, index) => {
                                return (
                                    <Flex direction="column" gap="2">
                                        <Text as="label" size="2" weight="bold" color={"gray"}>
                                            {capture.position ? `Stream at (${capture.position.x}, ${capture.position.y})` : 'Video Stream'}
                                        </Text>
                                        <div className="StreamCaptureEditPopup__select-wrapper">
                                            <button  className="StreamCaptureEditPopup__select"
                                                    onClick={async () => {
                                                        const screenStream = await navigator.mediaDevices.getDisplayMedia({
                                                            video: {frameRate: 60},
                                                            audio: true
                                                        });

                                                        // ✅ Create a real video element
                                                        const videoEl = document.createElement('video');
                                                        videoEl.srcObject = screenStream;
                                                        videoEl.muted = true;
                                                        videoEl.autoplay = true;

                                                        // Wait for it to have data before handing it off
                                                        await new Promise<void>(resolve => {
                                                            videoEl.onloadeddata = () => resolve();
                                                            videoEl.play().catch(console.error);
                                                        });

                                                        setVideoElement(index, videoEl);
                                                    }}>
                                                {capture.getSourceElement() ? 'Change Stream' : 'Select Stream'}
                                            </button>
                                        </div>
                                    </Flex>
                                )
                            })
                        }
                        {/* Audio Stream Select */}
                        <Flex direction="column" gap="2">
                            <Text as="label" size="2" weight="bold">
                                Microphone
                            </Text>
                            <div className="StreamCaptureEditPopup__select-wrapper">
                                {/*<select*/}
                                {/*    value={selectedAudio}*/}
                                {/*    onChange={(e) => handleAudioChange(e.target.value)}*/}
                                {/*    className="StreamCaptureEditPopup__select"*/}
                                {/*>*/}
                                {/*    {audioDevices.length === 0 ? (*/}
                                {/*        <option disabled>No microphones available</option>*/}
                                {/*    ) : (*/}
                                {/*        audioDevices.map(device => (*/}
                                {/*            <option key={device.deviceId} value={device.deviceId}>*/}
                                {/*                {device.label}*/}
                                {/*            </option>*/}
                                {/*        ))*/}
                                {/*    )}*/}
                                {/*</select>*/}
                                <ChevronDownIcon className="StreamCaptureEditPopup__select-icon"/>
                            </div>
                        </Flex>
                    </Flex>
                    <Flex direction="column" gap="5" style={{padding: '20px 0'}}>
                        Image Select
                        {
                            imageCaptures.map((capture, index) => {
                                return (
                                    <Flex direction="column" gap="2">
                                        <Text as="label" size="2" weight="bold" color={"gray"}>
                                            {capture.position ? `Stream at (${capture.position.x}, ${capture.position.y})` : 'Video Stream'}
                                        </Text>
                                        <div className="StreamCaptureEditPopup__select-wrapper">
                                            <button color={"red"} className="StreamCaptureEditPopup__select"
                                                    onClick={() => {
                                                        const input = document.createElement('input');
                                                        input.type = 'file';
                                                        input.accept = 'image/*';
                                                        // input.onchange = async () => {
                                                        //     const file = input.files?.[0];
                                                        //     if (!file) return;
                                                        //
                                                        //     const bitmap = await createImageBitmap(file);
                                                        //
                                                        //     const MAX = 2096;
                                                        //     const scale = Math.min(1, MAX / Math.max(bitmap.width, bitmap.height));
                                                        //
                                                        //     const canvas = document.createElement('canvas');
                                                        //     canvas.width = Math.floor(bitmap.width * scale);
                                                        //     canvas.height = Math.floor(bitmap.height * scale);
                                                        //     canvas.getContext('2d')!.drawImage(bitmap, 0, 0, canvas.width, canvas.height);
                                                        //     bitmap.close();
                                                        //
                                                        //     setImageElement(index, canvas);
                                                        // };
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
                                                    }}>
                                                {capture.getSourceElement() ? 'Change Stream' : 'Select Stream'}
                                            </button>
                                        </div>
                                    </Flex>
                                )
                            })

                        }


                        {/* Audio Stream Select */}
                        <Flex direction="column" gap="2">
                            <Text as="label" size="2" weight="bold" color={"blue"}>
                                Audio Select
                            </Text>
                            <div className="StreamCaptureEditPopup__select-wrapper">
                                {
                                    audioCaptures.map((capture, index) => {
                                        return (
                                            <button color={"red"} className="StreamCaptureEditPopup__select"
                                                    onClick={async () => {
                                                        const audioStream = await navigator.mediaDevices.getUserMedia({
                                                            audio: true,
                                                            video: false
                                                        });

                                                        setAudioStream(index, audioStream);
                                                    }}>
                                                {capture.getMediaStream() ? 'Change Audio Stream' : 'Select Audio Stream'}
                                            </button>
                                        )
                                    })
                                }
                                <ChevronDownIcon className="StreamCaptureEditPopup__select-icon"/>
                            </div>
                        </Flex>
                    </Flex>
                    <div className="StreamCaptureEditPopup__footer">
                        <Dialog.Close asChild>
                            <Button variant="soft" color="gray">
                                Cancel
                            </Button>
                        </Dialog.Close>
                        <Button onClick={handleConfirm} color="blue">
                            Confirm
                        </Button>
                    </div>
                </Dialog.Content>
            </Dialog.Portal>
        </Dialog.Root>
    );
};

export default StreamCaptureEditPopup;
