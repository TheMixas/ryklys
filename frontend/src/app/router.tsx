import { createBrowserRouter, RouterProvider } from 'react-router-dom';

import { paths } from '@/config/paths';

import AppRoot from './routes/app/root';
import Landing from './routes/landing';
// import Register from './routes/auth/register';
import StreamerDashboard from './routes/app/streamer-dashboard/streamer-dashboard';
import NotFound from './routes/not-found';
import StreamView from "@/features/stream-viewing/components/StreamView.tsx";
import BrowseStreamsPage from "@/app/routes/app/stream-view/BrowseStreamsPage.tsx";
import RegisterPage from "@/app/routes/auth/RegisterPage.tsx";
import LoginPage from "@/app/routes/auth/LoginPage.tsx";
import MePage from "@/app/routes/app/me/MePage.tsx";
import RequireAuth from "@/components/RequireAuth.tsx";

const router = createBrowserRouter([
    { path: paths.home.path, element: <Landing /> },
    // { path: paths.auth.register.path, element: <Register /> },
    { path: paths.auth.login.path, element: <LoginPage/> },
    { path: paths.auth.register.path, element: <RegisterPage /> },
    {
        path: paths.app.root.path,
        element: <AppRoot />, // add auth wrapper here if needed
        children: [
            { path: paths.app.streamerDashboard.path,     element: <RequireAuth><StreamerDashboard /></RequireAuth> },
            { path: paths.app.streamView.path,     element: <RequireAuth><StreamView /></RequireAuth> },
            { path: paths.app.streamBrowsing.path, element: <BrowseStreamsPage/>},
            { path: paths.app.me.path, element: <MePage/>}

        ],
    },
    { path: '*', element: <NotFound /> },
]);

export const AppRouter = () => <RouterProvider router={router} />
