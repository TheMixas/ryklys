import {
    Heading, Text, Flex, Box, Badge, ScrollArea, TextField, IconButton
} from '@radix-ui/themes';
import {useState, useRef, useEffect} from 'react';
import {PaperPlaneIcon} from '@radix-ui/react-icons';

export interface ChatMessage {
    id: string;
    username: string;
    text: string;
    createdAt: string;
}

const usernameColor = (name: string): string => {
    const colors = ['#e05c5c', '#5c9ce0', '#5ce09a', '#c45ce0', '#e0a85c', '#5ce0d4'];
    let hash = 0;
    for (const ch of name) hash = ch.charCodeAt(0) + ((hash << 5) - hash);
    return colors[Math.abs(hash) % colors.length];
};

interface ChatPanelProps {
    messages: ChatMessage[];
    viewerCount: number;
    wsRef: React.RefObject<WebSocket | null>;
    title?: string;
}

const ChatPanel = ({messages, viewerCount, wsRef, title = 'Stream Chat'}: ChatPanelProps) => {
    const chatBottomRef = useRef<HTMLDivElement>(null);
    const [inputValue, setInputValue] = useState('');

    useEffect(() => {
        chatBottomRef.current?.scrollIntoView({behavior: 'smooth'});
    }, [messages]);

    const handleSendMessage = () => {
        const text = inputValue.trim();
        if (!text || !wsRef.current || wsRef.current.readyState !== WebSocket.OPEN) return;

        wsRef.current.send(JSON.stringify({type: 'chat', text}));
        setInputValue('');
    };

    return (
        <Flex
            direction="column"
            gap="3"
            style={{
                width: '100%',
                height: '100%',
            }}
        >
            <Flex align="center" justify="between">
                <Heading size="4">{title}</Heading>
                <Badge color="green" size="1" radius="full">
                    {viewerCount} watching
                </Badge>
            </Flex>

            {/* Messages */}
            <Box
                style={{
                    background: 'var(--gray-2)',
                    borderRadius: 'var(--radius-3)',
                    flex: 1,
                    minHeight: '420px',
                    maxHeight: '520px',
                    overflow: 'hidden',
                }}
            >
                <ScrollArea style={{height: '100%', padding: '12px'}}>
                    <Flex direction="column" gap="2" p="2">
                        {messages.length === 0 && (
                            <Text size="2" color="gray" align="center" style={{padding: '20px 0'}}>
                                No messages yet. Say something!
                            </Text>
                        )}
                        {messages.map(msg => (
                            <Flex key={msg.id} gap="2" align="start">
                                <Text
                                    size="2"
                                    style={{
                                        color: usernameColor(msg.username),
                                        fontWeight: 600,
                                        whiteSpace: 'nowrap'
                                    }}
                                >
                                    {msg.username}:
                                </Text>
                                <Text size="2" style={{wordBreak: 'break-word'}}>
                                    {msg.text}
                                </Text>
                            </Flex>
                        ))}
                        <div ref={chatBottomRef}/>
                    </Flex>
                </ScrollArea>
            </Box>

            {/* Input */}
            <Flex gap="2">
                <TextField.Root
                    style={{flex: 1}}
                    placeholder="Say something…"
                    value={inputValue}
                    onChange={e => setInputValue(e.target.value)}
                    onKeyDown={e => {
                        if (e.key === 'Enter') handleSendMessage();
                    }}
                />
                <IconButton onClick={handleSendMessage} disabled={!inputValue.trim()}>
                    <PaperPlaneIcon/>
                </IconButton>
            </Flex>
        </Flex>
    );
};

export default ChatPanel;
