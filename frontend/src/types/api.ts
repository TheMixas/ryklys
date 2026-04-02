// ssynq this up with backend automatically using openapi-generator-cli

export type BaseEntity = {
    id: string;
    createdAt: number;
};
export interface LoginResponse {
    token: string;
}

export interface AuthUser {
    id: number;
    username: string;
}
export type Entity<T> = {
    [K in keyof T]: T[K];
} & BaseEntity;

export type Meta = {
    page: number;
    total: number;
    totalPages: number;
};

export interface User {
    id: number;
    username: string;
}

export type AuthResponse = {
    jwt: string;
    user: User;
};

export type Team = Entity<{
    name: string;
    description: string;
}>;

export type Discussion = Entity<{
    title: string;
    body: string;
    teamId: string;
    author: User;
}>;

export type Comment = Entity<{
    body: string;
    discussionId: string;
    author: User;
}>;