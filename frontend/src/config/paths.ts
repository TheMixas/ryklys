export const paths = {
    home: {
        path: '/',
        getHref: () => '/',
    },

    auth: {
        register: {
            path: '/auth/register',
            getHref: (redirectTo?: string | null | undefined) =>
                `/auth/register${redirectTo ? `?redirectTo=${encodeURIComponent(redirectTo)}` : ''}`,
        },
        login: {
            path: '/auth/login',
            getHref: (redirectTo?: string | null | undefined) =>
                `/auth/login${redirectTo ? `?redirectTo=${encodeURIComponent(redirectTo)}` : ''}`,
        },
    },

    app: {
        root: {
            path: '/app',
            getHref: () => '/app',
        },
        dashboard: {
            path: '',
            getHref: () => '/app',
        },
        streamerDashboard: {
            path: '/app/s-dashboard',
            getHref: () => '/app/s-dashboard',
        },
        streamBrowsing:{
            path: '/app/streams',
            getHref: () => '/app/streams',
        },
        me:{
            path: '/app/me',
            getHref: () => '/app/me',
        },
        streamView :{
            path: '/app/viewStream/:username',
            getHref: () => '/app/viewStream',
        },
        discussion: {
            path: 'discussions/:discussionId',
            getHref: (id: string) => `/app/discussions/${id}`,
        },
        users: {
            path: 'users',
            getHref: () => '/app/users',
        },
        profile: {
            path: 'profile',
            getHref: () => '/app/profile',
        },
    },
} as const;