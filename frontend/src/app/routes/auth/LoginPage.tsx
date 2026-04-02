import {
    Heading, Text, Flex, Box, Button, TextField, Separator, IconButton
} from '@radix-ui/themes';
import {useState} from 'react';
import {useNavigate} from 'react-router';
import {EnterIcon, EyeOpenIcon, EyeClosedIcon} from '@radix-ui/react-icons';
import {env} from "@/config/env.ts";
import {useAuth} from "@/hooks/useAuth.tsx";

const LoginPage = () => {
    const navigate = useNavigate();
    const [username, setUsername] = useState('');
    const [password, setPassword] = useState('');
    const [showPassword, setShowPassword] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [loading, setLoading] = useState(false);
    const { refresh } = useAuth();

    const handleLogin = async () => {
        setError(null);
        setLoading(true);

        try {
            const res = await fetch(`${env.API_URL}/api/users/login`, {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                credentials: 'include',
                body: JSON.stringify({username, password}),
            });

            const text = await res.text();

            if (res.ok) {
                // Cookie is set automatically via Set-Cookie header
                await refresh();
                navigate('/');
            } else {
                setError(text || 'Login failed');
            }
        } catch (e) {
            console.error('[Login] Request failed:', e);
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
                    <Heading size="7">Welcome Back</Heading>
                    <Text size="3" color="gray">
                        Log in to your Ryklys account.
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
                            placeholder="Your username"
                            value={username}
                            onChange={e => setUsername(e.target.value)}
                        />
                    </Flex>

                    <Flex direction="column" gap="1">
                        <Text size="2" weight="bold">Password</Text>
                        <Flex gap="2">
                            <TextField.Root
                                style={{flex: 1}}
                                type={showPassword ? 'text' : 'password'}
                                placeholder="Your password"
                                value={password}
                                onChange={e => setPassword(e.target.value)}
                                onKeyDown={e => {
                                    if (e.key === 'Enter') handleLogin();
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
                    onClick={handleLogin}
                    disabled={loading || !username.trim() || !password.trim()}
                >
                    {loading ? (
                        <Text size="2">Logging in…</Text>
                    ) : (
                        <>
                            <EnterIcon />
                            Log In
                        </>
                    )}
                </Button>

                {/* Register link */}
                <Flex align="center" justify="center" gap="1">
                    <Text size="2" color="gray">Don't have an account?</Text>
                    <Text
                        size="2"
                        style={{
                            color: 'var(--accent-9)',
                            cursor: 'pointer',
                            fontWeight: 600,
                        }}
                        onClick={() => navigate('/register')}
                    >
                        Sign up
                    </Text>
                </Flex>
            </Flex>
        </Flex>
    );
};

export default LoginPage;
