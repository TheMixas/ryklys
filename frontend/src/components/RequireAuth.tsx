// src/components/RequireAuth.tsx
import {useAuth} from "@/hooks/useAuth.tsx";
import {Navigate} from "react-router";

const RequireAuth = ({children}: {children: React.ReactNode}) => {
    const {authenticated} = useAuth();

    if (!authenticated) {
        return <Navigate to="/auth/login" replace />;
    }

    return <>{children}</>;
};

export default RequireAuth;