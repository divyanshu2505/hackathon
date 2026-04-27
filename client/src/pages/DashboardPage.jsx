import { useEffect, useState } from 'react';
import { lifeosApi } from '../api/client.js';
import { useAuth } from '../context/AuthContext.jsx';

export default function DashboardPage() {
  const { token, user } = useAuth();
  const [dashboard, setDashboard] = useState(null);
  const [chatReply, setChatReply] = useState('');
  const [ideas, setIdeas] = useState([]);
  const [summary, setSummary] = useState(null);
  const [book, setBook] = useState(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    async function load() {
      const data = await lifeosApi.dashboard(token);
      setDashboard(data);
    }
    load();
  }, [token]);

  async function run(task) {
    setLoading(true);
    try {
      await task();
      const fresh = await lifeosApi.dashboard(token);
      setDashboard(fresh);
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="dashboard-grid">
      <section className="card">
        <h2>Hello {user?.name}</h2>
        <p>One app to learn, earn, and save time using AI.</p>
        <div className="metrics">
          <div><strong>{dashboard?.metrics?.chatSessions || 0}</strong><span>Chats</span></div>
          <div><strong>{dashboard?.metrics?.summariesSaved || 0}</strong><span>Summaries</span></div>
          <div><strong>{dashboard?.metrics?.earningsIdeasViewed || 0}</strong><span>Earning Sessions</span></div>
        </div>
      </section>

      <section className="card">
        <h3>AI Chat Assistant</h3>
        <button
          disabled={loading}
          onClick={() => run(async () => {
            const result = await lifeosApi.chat(token, 'Give me a 7-day productivity + earning plan.');
            setChatReply(result.reply);
          })}
        >
          Generate plan
        </button>
        {chatReply && <p>{chatReply}</p>}
      </section>

      <section className="card">
        <h3>AI Revenue Generator</h3>
        <button disabled={loading} onClick={() => run(async () => setIdeas((await lifeosApi.revenue(token)).ideas))}>
          Fetch earning ideas
        </button>
        <ul>
          {ideas.map((idea) => (
            <li key={idea.id}>{idea.idea}</li>
          ))}
        </ul>
      </section>

      <section className="card">
        <h3>Video Summary Tool</h3>
        <button
          disabled={loading}
          onClick={() => run(async () => setSummary((await lifeosApi.videoSummary(token, 'React roadmap')).summary))}
        >
          Summarize topic
        </button>
        {summary && (
          <>
            <p>{summary.notes}</p>
            <ul>{summary.keyPoints.map((point) => <li key={point}>{point}</li>)}</ul>
          </>
        )}
      </section>

      <section className="card">
        <h3>Book Generator</h3>
        <button disabled={loading} onClick={() => run(async () => setBook((await lifeosApi.book(token, 'AI freelancing')).book))}>
          Create draft
        </button>
        {book && (
          <>
            <p><strong>{book.title}</strong></p>
            <p>{book.quickSummary}</p>
          </>
        )}
      </section>

      <section className="card">
        <h3>Recent Activity</h3>
        <ul>
          {(dashboard?.recent || []).map((item) => (
            <li key={item._id}>{item.title}</li>
          ))}
        </ul>
      </section>
    </div>
  );
}
