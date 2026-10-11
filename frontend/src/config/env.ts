export const env = {
    // Production builds are served by Caddy on the same origin as the API,
    // so use the page's own origin (https -> wss for WebSockets).
    API_URL: import.meta.env.PROD
        ? window.location.origin
        : (import.meta.env.VITE_API_URL || 'http://localhost:8080'),
    MOCK_AUTH: import.meta.env.VITE_MOCK_AUTH === 'true',
    STREAM_POCKET_URL: '',
};