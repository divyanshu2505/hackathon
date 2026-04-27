import { Activity } from '../models/Activity.js';
import { User } from '../models/User.js';
import {
  generateBook,
  generateChatResponse,
  generateVideoSummary,
  getRevenueIdeas
} from '../services/aiMockService.js';

export async function dashboard(req, res) {
  const [user, recent] = await Promise.all([
    User.findById(req.user.sub).select('name email interests'),
    Activity.find({ userId: req.user.sub }).sort({ createdAt: -1 }).limit(10)
  ]);

  return res.json({
    user,
    metrics: {
      chatSessions: recent.filter((item) => item.type === 'chat').length,
      summariesSaved: recent.filter((item) => item.type === 'summary').length,
      earningsIdeasViewed: recent.filter((item) => item.type === 'earning').length
    },
    recent
  });
}

export async function chat(req, res) {
  const { message } = req.body;
  if (!message) return res.status(400).json({ message: 'message is required' });

  const user = await User.findById(req.user.sub).select('interests');
  const reply = generateChatResponse(message, user?.interests ?? []);

  const activity = await Activity.create({
    userId: req.user.sub,
    type: 'chat',
    title: 'AI Chat Prompt',
    payload: { message, reply }
  });

  return res.json({ reply, activityId: activity._id });
}

export async function revenue(req, res) {
  const ideas = getRevenueIdeas();
  await Activity.create({
    userId: req.user.sub,
    type: 'earning',
    title: 'Revenue ideas fetched',
    payload: { count: ideas.length }
  });

  return res.json({ ideas });
}

export async function summarizeVideo(req, res) {
  const { topic } = req.body;
  if (!topic) return res.status(400).json({ message: 'topic is required' });

  const summary = generateVideoSummary(topic);
  const saved = await Activity.create({
    userId: req.user.sub,
    type: 'summary',
    title: `Video summary: ${topic}`,
    payload: summary
  });

  return res.json({ summary, activityId: saved._id });
}

export async function book(req, res) {
  const { topic } = req.body;
  if (!topic) return res.status(400).json({ message: 'topic is required' });

  const bookDraft = generateBook(topic);
  const saved = await Activity.create({
    userId: req.user.sub,
    type: 'book',
    title: `Book draft: ${topic}`,
    payload: bookDraft
  });

  return res.json({ book: bookDraft, activityId: saved._id });
}
