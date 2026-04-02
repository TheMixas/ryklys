export const env = {
    API_URL: import.meta.env.VITE_API_URL || 'http://localhost:8080',
    MOCK_AUTH: import.meta.env.VITE_MOCK_AUTH === 'true',
    STREAM_POCKET_URL: 'http://localhost:7070',
};