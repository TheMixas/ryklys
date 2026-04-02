import { NavigationMenu } from "radix-ui";
import { Flex, Text, Button } from "@radix-ui/themes";
import { useNavigate, useLocation } from "react-router";
import { paths } from "@/config/paths";
import sharkLogo from "@/assets/funny_ryklys.svg";
import { useAuth } from "@/hooks/useAuth.tsx";
import { env } from "@/config/env.ts";

const navItems = [
    { label: "DISCOVER", href: paths.app.streamBrowsing.getHref() },
    { label: "STREAM", href: paths.app.streamerDashboard.getHref() },
    { label: "ME", href: paths.app.me.getHref() },
] as const;

export const Navbar = () => {
    const navigate = useNavigate();
    const location = useLocation();
    const { authenticated, user, refresh } = useAuth();

    return (
        <Flex
            align="center"
            justify="between"
            width="100%"
            style={{
                padding: "8px 24px",
                borderBottom: "1px solid var(--gray-6)",
                backgroundColor: "var(--color-background)",
                flexShrink: 0,
            }}
        >
            {/* Left — logo */}
            <Flex
                align="center"
                gap="2"
                style={{ cursor: "pointer" }}
                onClick={() => navigate(paths.home.getHref())}
            >
                <img src={sharkLogo} alt="Ryklys" style={{ width: "28px", height: "28px" }} />
                <Text size="3" weight="bold">Ryklys</Text>
            </Flex>

            {/* Center — nav links */}
            <NavigationMenu.Root>
                <NavigationMenu.List
                    style={{
                        display: "flex",
                        listStyle: "none",
                        margin: 0,
                        padding: 0,
                        gap: "4px",
                    }}
                >
                    {navItems.map((item) => {
                        const isActive = location.pathname === item.href;
                        return (
                            <NavigationMenu.Item key={item.label}>
                                <NavigationMenu.Link
                                    active={isActive}
                                    onSelect={(e) => {
                                        e.preventDefault();
                                        navigate(item.href);
                                    }}
                                    style={{
                                        display: "block",
                                        padding: "8px 16px",
                                        borderRadius: "6px",
                                        textDecoration: "none",
                                        fontSize: "14px",
                                        fontWeight: 600,
                                        letterSpacing: "0.5px",
                                        cursor: "pointer",
                                        color: isActive ? "var(--accent-11)" : "var(--gray-11)",
                                        backgroundColor: isActive ? "var(--accent-3)" : "transparent",
                                    }}
                                >
                                    {item.label}
                                </NavigationMenu.Link>
                            </NavigationMenu.Item>
                        );
                    })}
                </NavigationMenu.List>
            </NavigationMenu.Root>

            {/* Right — auth */}
            <Flex align="center" gap="3">
                {authenticated ? (
                    <>
                        <Text size="2">Welcome, <strong>{user?.username}</strong></Text>
                        <Button
                            variant="soft"
                            size="1"
                            onClick={async () => {
                                await fetch(`${env.API_URL}/api/users/logout`, {
                                    method: 'POST',
                                    credentials: 'include',
                                });
                                await refresh();
                                navigate('/');
                            }}
                        >
                            Log out
                        </Button>
                    </>
                ) : (
                    <>
                        <Button variant="soft" size="1" onClick={() => navigate('/auth/login')}>
                            Log in
                        </Button>
                        <Button size="1" onClick={() => navigate('/auth/register')}>
                            Sign up
                        </Button>
                    </>
                )}
            </Flex>
        </Flex>
    );
};