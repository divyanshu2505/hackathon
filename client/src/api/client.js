const API_BASE = import.meta.env.VITE_API_BASE_URL || '';

async function request(path, options = {}) {
  const response = await fetch(`${API_BASE}${path}`, {
    headers: { 'Content-Type': 'application/json', ...(options.headers || {}) },
    ...options
  });

  const data = await response.json();
  if (!response.ok) {
    throw new Error(data.message || 'Request failed');
  }

  return data;
}

export const authApi = {
  login(email, password) {
    return request('/api/auth/login', {
      method: 'POST',
      body: JSON.stringify({ email, password })
    });
  },
  signup(payload) {
    return request('/api/auth/signup', {
      method: 'POST',
      body: JSON.stringify(payload)
    });
  }
};

export const lifeosApi = {
  dashboard(token) {
    return request('/api/lifeos/dashboard', {
      headers: { Authorization: `Bearer ${token}` }
    });
  },
  chat(token, message) {
    return request('/api/lifeos/chat', {
      method: 'POST',
      headers: { Authorization: `Bearer ${token}` },
      body: JSON.stringify({ message })
    });
  },
  revenue(token) {
    return request('/api/lifeos/revenue', {
      headers: { Authorization: `Bearer ${token}` }
    });
  },
  videoSummary(token, topic) {
    return request('/api/lifeos/summary/video', {
      method: 'POST',
      headers: { Authorization: `Bearer ${token}` },
      body: JSON.stringify({ topic })
    });
  },
  book(token, topic) {
    return request('/api/lifeos/book', {
      method: 'POST',
      headers: { Authorization: `Bearer ${token}` },
      body: JSON.stringify({ topic })
    });
  }
};
