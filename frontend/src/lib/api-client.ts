import Axios from 'axios';
import { env } from '@/config/env';


export const api = Axios.create({
    baseURL: env.API_URL,
    withCredentials: true, // send cookies
});

// api.interceptors.response.use(
//     (response) => response.data,
//     (error) => {
//         const message = error.response?.data?.message || error.message;
//         toast.error(message);
//         if (error.response?.status === 401) {
//             // server should clear cookie or client can redirect and let server handle session invalidation
//             window.location.href = paths.auth.login.getHref(window.location.pathname);
//         }
//         return Promise.reject(error);
//     }
// );