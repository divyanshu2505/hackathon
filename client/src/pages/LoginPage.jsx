import { useState } from 'react';
import { Navigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext.jsx';

export default function LoginPage() {
  const { token, login, signup } = useAuth();
  const [isSignup, setIsSignup] = useState(false);
  const [error, setError] = useState('');
  const [form, setForm] = useState({ name: '', email: '', password: '', interests: 'AI, freelancing' });

  if (token) return <Navigate to="/" replace />;

  async function submit(e) {
    e.preventDefault();
    setError('');

    try {
      if (isSignup) {
        await signup({
          name: form.name,
          email: form.email,
          password: form.password,
          interests: form.interests.split(',').map((item) => item.trim())
        });
      } else {
        await login(form.email, form.password);
      }
    } catch (err) {
      setError(err.message);
    }
  }

  return (
    <section className="card auth-card">
      <h2>{isSignup ? 'Create account' : 'Welcome back'}</h2>
      <p>Learn, earn, and stay productive in one place.</p>
      <form onSubmit={submit}>
        {isSignup && (
          <input
            placeholder="Name"
            value={form.name}
            onChange={(e) => setForm((prev) => ({ ...prev, name: e.target.value }))}
          />
        )}
        <input
          type="email"
          placeholder="Email"
          value={form.email}
          onChange={(e) => setForm((prev) => ({ ...prev, email: e.target.value }))}
        />
        <input
          type="password"
          placeholder="Password"
          value={form.password}
          onChange={(e) => setForm((prev) => ({ ...prev, password: e.target.value }))}
        />
        {isSignup && (
          <input
            placeholder="Interests (comma separated)"
            value={form.interests}
            onChange={(e) => setForm((prev) => ({ ...prev, interests: e.target.value }))}
          />
        )}
        {error && <p className="error">{error}</p>}
        <button type="submit">{isSignup ? 'Sign up' : 'Login'}</button>
      </form>

      <button className="link-btn" onClick={() => setIsSignup((prev) => !prev)}>
        {isSignup ? 'Already have an account? Login' : 'New user? Create account'}
      </button>
    </section>
  );
}
