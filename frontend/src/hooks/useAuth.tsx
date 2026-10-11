import React, { createContext, useContext, useEffect, useState, useCallback } from "react";
import {env} from "@/config/env.ts";
import type {User} from "@/types/api.ts";

const AuthContext = createContext<{
    authenticated: boolean;
    user: User | null;
    // true until the first /api/users/me check finishes. Before that we don't
    // know yet whether the cookie is valid, so pages must not treat it as "logged out".
    loading: boolean;
    refresh: () => Promise<void>;
}>({ authenticated: false, user: null, loading: true, refresh: async () => {} });

export function AuthProvider({ children }: { children: React.ReactNode }) {
    const [authenticated, setAuthenticated] = useState<boolean>(false);
    const [user, setUser] = useState<User | null>(null);
    const [loading, setLoading] = useState<boolean>(true);

    const refresh = useCallback(async () => {
        try {
            const r = await fetch(env.API_URL + "/api/users/me", { credentials: "include" });
            if (r.ok) {
                const tempUser = await r.json();
                setAuthenticated(true);
                setUser(tempUser);
            } else {
                setAuthenticated(false);
                setUser(null);
            }
        } catch {
            setAuthenticated(false);
            setUser(null);
        } finally {
            setLoading(false);
        }
    }, []);

    useEffect(() => {
        refresh();
    }, [refresh]);

    return (
        <AuthContext.Provider value={{ authenticated, user, loading, refresh }}>
            {children}
        </AuthContext.Provider>
    );
}

export function useAuth() {
    return useContext(AuthContext);
}