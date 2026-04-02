// import { configureAuth } from 'react-query-auth';
// import { Navigate, useLocation } from 'react-router';
// import { z } from 'zod';
//
// import { paths } from '@/config/paths';
// import type {AuthUser} from '@/types/api';
//
// import { api } from './api-client';
//
//
//
// const TOKEN_KEY = 'auth_token';
//
// export const getToken = () => localStorage.getItem(TOKEN_KEY);
// export const setToken = (token: string) => localStorage.setItem(TOKEN_KEY, token);
// export const removeToken = () => localStorage.removeItem(TOKEN_KEY);
//
// // api call definitions for auth (types, schemas, requests):
// // these are not part of features as this is a module shared across features
//
// const getUser = async (): Promise<AuthUser | null> => {
//     const token = getToken();
//     if (!token) return null;
//
//     try {
//         const response = await api.get('/api/users/me');
//         return response; // your interceptor already unwraps .data
//     } catch {
//         removeToken();
//         return null;
//     }
// };
//
// const logout = async (): Promise<void> => {
//     // Your backend doesn't have a logout endpoint (JWT is stateless)
//     // Just clear the token client-side
//     removeToken();
// };
//
// // ── Login ──
// // eslint-disable-next-line react-refresh/only-export-components
// export const loginInputSchema = z.object({
//     username: z.string().min(1, 'Username is required'),
//     password: z.string().min(5, 'Password must be at least 5 characters'),
// });
//
// export type LoginInput = z.infer<typeof loginInputSchema>;
//
// // eslint-disable-next-line react-refresh/only-export-components
// export const loginWithCredentials = async (data: LoginInput): Promise<AuthUser> => {
//     const response = await api.post('/api/users/login', data);
//     console.log("Login response:", response);
//     //setToken(response.t
//     // After getting token, fetch user info
//     const user = await getUser();
//     if (!user) throw new Error('Failed to get user after login');
//     return user;
// };
//
// // ── Register ──
// // eslint-disable-next-line react-refresh/only-export-components
// export const registerInputSchema = z.object({
//     username: z.string().min(1, 'Username is required'),
//     email: z.string().min(1, 'Email is required').email('Invalid email'),
//     password: z.string().min(5, 'Password must be at least 5 characters'),
// });
//
// export type RegisterInput = z.infer<typeof registerInputSchema>;
//
// const registerUser = async (data: RegisterInput): Promise<AuthUser> => {
//     await api.post('/api/users/register', data);
//     // After registration, log them in automatically
//     const user = await loginWithCredentials({
//         username: data.username,
//         password: data.password,
//     });
//     return user;
// };
//
//
// // const loginWithEmailAndPassword = (data: LoginInput): Promise<AuthResponse> => {
// //     if (env.MOCK_AUTH) {
// //         //implement simple email/password check for mock auth
// //     }
// //
// //     return api.post('/auth/login', data);
// // };
//
//
//
// // const registerWithEmailAndPassword = (
// //     data: RegisterInput,
// // ): Promise<AuthResponse> => {
// //     if (env.MOCK_AUTH) {
// //         //implement simple registration logic for mock auth
// //     }
// //
// //     return api.post('/auth/register', data);
// // };
//
// // ──────────────────────────────────────────────
// // react-query-auth config
// // ──────────────────────────────────────────────
// const authConfig = {
//     userFn: getUser,
//     loginFn: loginWithCredentials,
//     registerFn: registerUser,
//     logoutFn: logout,
// };
//
// // eslint-disable-next-line react-refresh/only-export-components
// export const { useUser, useLogin, useLogout, useRegister, AuthLoader } =
//     configureAuth(authConfig);
//
// export const ProtectedRoute = ({ children }: { children: React.ReactNode }) => {
//     const user = useUser();
//     const location = useLocation();
//
//     if (!user.data) {
//         return (
//             <Navigate to={paths.auth.login.getHref(location.pathname)} replace />
//         );
//     }
//
//     return children;
// };