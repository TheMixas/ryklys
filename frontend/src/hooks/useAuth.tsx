import React, { createContext, useContext, useEffect, useState, useCallback } from "react";
import {env} from "@/config/env.ts";
import type {User} from "@/types/api.ts";

const AuthContext = createContext<{
    authenticated: boolean;
    user: User | null;
    refresh: () => Promise<void>;
}>({ authenticated: false, user: null, refresh: async () => {} });

export function AuthProvider({ children }: { children: React.ReactNode }) {
    const [authenticated, setAuthenticated] = useState<boolean>(false);
    const [user, setUser] = useState<User | null>(null);

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
        }
    }, []);

    useEffect(() => {
        refresh();
    }, [refresh]);

    return (
        <AuthContext.Provider value={{ authenticated, user, refresh }}>
            {children}
        </AuthContext.Provider>
    );
}

export function useAuth() {
    return useContext(AuthContext);
}