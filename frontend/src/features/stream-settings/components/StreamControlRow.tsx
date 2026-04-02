import { Flex, Button } from '@radix-ui/themes';

interface StreamControlRowProps {
    onPlay: () => void;
    onPause: () => void;
    onStop: () => void;
}

const StreamControlRow = ({ onPlay, onPause, onStop }: StreamControlRowProps) => {
    return (
        <Flex gap="3" justify="center">
            <Button onClick={onPlay}>Play</Button>
            <Button onClick={onPause}>Pause</Button>
            <Button onClick={onStop}>Stop</Button>
        </Flex>
    );
};

export default StreamControlRow;
