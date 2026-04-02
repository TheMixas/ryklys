import {ReactQueryDevtools} from '@tanstack/react-query-devtools';
import * as React from 'react';
import {ErrorBoundary} from 'react-error-boundary';
import {MainErrorFallback} from '@/components/errors/main';
import {HelmetProvider} from "react-helmet-async";
import {ToastContainer} from "react-toastify";
import {Spinner, Theme, ThemePanel} from "@radix-ui/themes";

type AppProviderProps = {
    children: React.ReactNode;
};

export const AppProvider = ({children}: AppProviderProps) => {

    return (
        <React.Suspense
            fallback={
                <div className="flex h-screen w-screen items-center justify-center">
                    <Spinner size={"3"}/>
                </div>
            }
        >
            <Theme appearance={"dark"}>
                <ErrorBoundary FallbackComponent={MainErrorFallback}>
                    <HelmetProvider>
                        {import.meta.env.DEV && <ReactQueryDevtools/>}
                        <ToastContainer/>
                        <ThemePanel/>
                            {children}
                    </HelmetProvider>
                </ErrorBoundary>
            </Theme>
        </React.Suspense>
    );
};