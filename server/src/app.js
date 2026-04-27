import cors from 'cors';
import express from 'express';
import authRoutes from './routes/authRoutes.js';
import lifeosRoutes from './routes/lifeosRoutes.js';

const app = express();

app.use(cors());
app.use(express.json());

app.get('/api/health', (_req, res) => {
  res.json({ status: 'ok', service: 'lifeos-ai-server' });
});

app.use('/api/auth', authRoutes);
app.use('/api/lifeos', lifeosRoutes);

app.use((err, _req, res, _next) => {
  console.error(err);
  res.status(500).json({ message: 'Internal server error' });
});

export default app;
