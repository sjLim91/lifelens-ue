declare module '@appdeploy/client' {
  interface ApiResponse<T = unknown> {
    data: T;
  }

  interface ApiClient {
    get<T = unknown>(url: string, data?: unknown): Promise<ApiResponse<T>>;
    post<T = unknown>(url: string, data?: unknown): Promise<ApiResponse<T>>;
    put<T = unknown>(url: string, data?: unknown): Promise<ApiResponse<T>>;
    delete<T = unknown>(url: string, data?: unknown): Promise<ApiResponse<T>>;
  }

  export const api: ApiClient;
}
