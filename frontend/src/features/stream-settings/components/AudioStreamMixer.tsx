import { Flex, Text, Slider, IconButton, Card } from '@radix-ui/themes';
import { SpeakerLoudIcon, SpeakerOffIcon } from '@radix-ui/react-icons';
import { useState } from 'react';
import type { AudioCapture } from "@/features/stream/models/AudioCapture.ts";

interface StreamAudioMixerProps {
    audioCaptures: AudioCapture[];
}

const AudioTrackControl = ({ capture, index }: { capture: AudioCapture, index: number }) => {
    // 1. Check if the gain node (and thus the audio stream) actually exists
    const isDisabled = !capture.gainNode;

    // Default to 100% volume. We multiply the Web Audio Gain (0.0 to 1.0) by 100 for the UI slider.
    const initialVolume = capture.gainNode ? capture.gainNode.gain.value * 100 : 100;
    const [volume, setVolume] = useState(initialVolume);
    const [isMuted, setIsMuted] = useState(initialVolume === 0);
    const [prevVolume, setPrevVolume] = useState(initialVolume || 100);

    const handleVolumeChange = (value: number[]) => {
        if (isDisabled) return; // Extra safety check

        const newVol = value[0];
        setVolume(newVol);
        setIsMuted(newVol === 0);

        // Apply the volume to the actual Web Audio GainNode (scale 0 to 1)
        if (capture.gainNode) {
            capture.setVolume(newVol / 100);
        }
    };

    const toggleMute = () => {
        if (isDisabled) return;

        if (isMuted) {
            handleVolumeChange([prevVolume === 0 ? 100 : prevVolume]);
        } else {
            setPrevVolume(volume);
            handleVolumeChange([0]);
        }
    };

    return (
        // 2. Drop the opacity of the entire row if disabled to make it visually obvious
        <Flex align="center" gap="4" width="100%" style={{ opacity: isDisabled ? 0.5 : 1 }}>
            <IconButton
                size="2"
                variant={isMuted && !isDisabled ? "solid" : "soft"}
                color={isDisabled ? "gray" : (isMuted ? "red" : "gray")}
                onClick={toggleMute}
                disabled={isDisabled} // 3. Radix UI disabled prop
                style={{ cursor: isDisabled ? 'not-allowed' : 'pointer' }}
            >
                {isMuted ? <SpeakerOffIcon /> : <SpeakerLoudIcon />}
            </IconButton>

            <Flex direction="column" width="100%" gap="1">
                <Flex justify="between">
                    <Text size="2" weight="bold" color="gray">
                        {`Audio Source ${index + 1}`} {isDisabled && "(Empty)"}
                    </Text>
                    <Text size="2" color="gray">{Math.round(volume)}%</Text>
                </Flex>
                <Slider
                    value={[volume]}
                    max={100}
                    step={1}
                    onValueChange={handleVolumeChange}
                    disabled={isDisabled} // 4. Radix UI disabled prop
                />
            </Flex>
        </Flex>
    );
};

const StreamAudioMixer = ({ audioCaptures }: StreamAudioMixerProps) => {
    if (!audioCaptures || audioCaptures.length === 0) return null;

    return (
        <Card size="2" style={{ width: '100%' }}>
            <Flex direction="column" gap="4">
                <Text size="3" weight="bold">Audio Mixer</Text>
                {audioCaptures.map((capture, index) => (
                    <AudioTrackControl key={index} capture={capture} index={index} />
                ))}
            </Flex>
        </Card>
    );
};

export default StreamAudioMixer;