// src/components/RequireAuth.tsx
import {useAuth} from "@/hooks/useAuth.tsx";
import {Navigate} from "react-router";

const RequireAuth = ({children}: {children: React.ReactNode}) => {
    const {authenticated, loading} = useAuth();

    // Still checking the session cookie (e.g. right after the browser reopens).
    // Redirecting now would log out a user who is actually signed in.
    if (loading) {
        return null;
    }

    if (!authenticated) {
        return <Navigate to="/auth/login" replace />;
    }

    return <>{children}</>;
};

export default RequireAuth;