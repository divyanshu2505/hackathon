import { createContext, useContext, useMemo, useState } from 'react';
import { authApi } from '../api/client.js';

const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const [token, setToken] = useState(localStorage.getItem('lifeos_token') || '');
  const [user, setUser] = useState(() => {
    const raw = localStorage.getItem('lifeos_user');
    return raw ? JSON.parse(raw) : null;
  });

  async function login(email, password) {
    const data = await authApi.login(email, password);
    setToken(data.token);
    setUser(data.user);
    localStorage.setItem('lifeos_token', data.token);
    localStorage.setItem('lifeos_user', JSON.stringify(data.user));
  }

  async function signup(payload) {
    const data = await authApi.signup(payload);
    setToken(data.token);
    setUser(data.user);
    localStorage.setItem('lifeos_token', data.token);
    localStorage.setItem('lifeos_user', JSON.stringify(data.user));
  }

  function logout() {
    setToken('');
    setUser(null);
    localStorage.removeItem('lifeos_token');
    localStorage.removeItem('lifeos_user');
  }

  const value = useMemo(() => ({ token, user, login, signup, logout }), [token, user]);
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

export function useAuth() {
  return useContext(AuthContext);
}
