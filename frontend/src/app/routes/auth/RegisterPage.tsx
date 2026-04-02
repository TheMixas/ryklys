import {
    Heading, Text, Flex, Box, Button, TextField, Separator, IconButton
} from '@radix-ui/themes';
import {useState} from 'react';
import {useNavigate} from 'react-router';
import {RocketIcon, EyeOpenIcon, EyeClosedIcon} from '@radix-ui/react-icons';
import {env} from "@/config/env.ts";
import {useAuth} from "@/hooks/useAuth.tsx";

const RegisterPage = () => {
    const navigate = useNavigate();
    const [username, setUsername] = useState('');
    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [showPassword, setShowPassword] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [loading, setLoading] = useState(false);
    const { refresh } = useAuth();

    const handleRegister = async () => {
        setError(null);
        setLoading(true);

        try {
            const res = await fetch(`${env.API_URL}/api/users/register`, {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                credentials: 'include',
                body: JSON.stringify({username, email, password}),
            });

            const text = await res.text();

            if (res.ok) {
                // Registration successful — send them to home
                await refresh();
                navigate('/');
            } else {
                setError(text || 'Registration failed');
            }
        } catch (e) {
            console.error('[Register] Request failed:', e);
            setError('Could not reach server. Try again.');
        } finally {
            setLoading(false);
        }
    };

    return (
        <Flex
            align="center"
            justify="center"
            style={{minHeight: '100vh', padding: '24px'}}
        >
            <Flex
                direction="column"
                gap="5"
                style={{
                    width: '100%',
                    maxWidth: 420,
                    background: 'var(--gray-2)',
                    borderRadius: 'var(--radius-3)',
                    padding: '40px 32px',
                }}
            >
                {/* Header */}
                <Flex direction="column" align="center" gap="1">
                    <Heading size="7">Create Account</Heading>
                    <Text size="3" color="gray">
                        Join Ryklys and start streaming.
                    </Text>
                </Flex>

                <Separator size="4" />

                {/* Error message */}
                {error && (
                    <Box
                        style={{
                            background: 'var(--red-3)',
                            border: '1px solid var(--red-6)',
                            borderRadius: 'var(--radius-2)',
                            padding: '10px 14px',
                        }}
                    >
                        <Text size="2" style={{color: 'var(--red-11)'}}>{error}</Text>
                    </Box>
                )}

                {/* Form fields */}
                <Flex direction="column" gap="3">
                    <Flex direction="column" gap="1">
                        <Text size="2" weight="bold">Username</Text>
                        <TextField.Root
                            placeholder="Pick a username"
                            value={username}
                            onChange={e => setUsername(e.target.value)}
                        />
                    </Flex>

                    <Flex direction="column" gap="1">
                        <Text size="2" weight="bold">Email</Text>
                        <TextField.Root
                            type="email"
                            placeholder="you@example.com"
                            value={email}
                            onChange={e => setEmail(e.target.value)}
                        />
                    </Flex>

                    <Flex direction="column" gap="1">
                        <Text size="2" weight="bold">Password</Text>
                        <Flex gap="2">
                            <TextField.Root
                                style={{flex: 1}}
                                type={showPassword ? 'text' : 'password'}
                                placeholder="At least 8 characters"
                                value={password}
                                onChange={e => setPassword(e.target.value)}
                                onKeyDown={e => {
                                    if (e.key === 'Enter') handleRegister();
                                }}
                            />
                            <IconButton
                                variant="soft"
                                onClick={() => setShowPassword(p => !p)}
                            >
                                {showPassword ? <EyeClosedIcon /> : <EyeOpenIcon />}
                            </IconButton>
                        </Flex>
                    </Flex>
                </Flex>

                {/* Submit */}
                <Button
                    size="3"
                    style={{width: '100%'}}
                    onClick={handleRegister}
                    disabled={loading || !username.trim() || !email.trim() || !password.trim()}
                >
                    {loading ? (
                        <Text size="2">Creating account…</Text>
                    ) : (
                        <>
                            <RocketIcon />
                            Create Account
                        </>
                    )}
                </Button>

                {/* Login link */}
                <Flex align="center" justify="center" gap="1">
                    <Text size="2" color="gray">Already have an account?</Text>
                    <Text
                        size="2"
                        style={{
                            color: 'var(--accent-9)',
                            cursor: 'pointer',
                            fontWeight: 600,
                        }}
                        onClick={() => navigate('/login')}
                    >
                        Log in
                    </Text>
                </Flex>
            </Flex>
        </Flex>
    );
};

export default RegisterPage;
