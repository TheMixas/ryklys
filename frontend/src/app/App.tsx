import {AppRouter} from './router';
import {Theme, ThemePanel} from "@radix-ui/themes";
import {AuthProvider} from "@/hooks/useAuth.tsx";

export const ErrorBoundary = () => {
    return <div>Something went wrong!</div>;
};

function App() {
    return (<Theme appearance={"dark"}>
            <AuthProvider>
                <AppRouter/>
                <ThemePanel/>
            </AuthProvider>

        </Theme>
    );
}

export default App
