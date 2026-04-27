import { Router } from 'express';
import { book, chat, dashboard, revenue, summarizeVideo } from '../controllers/lifeosController.js';
import { requireAuth } from '../middleware/auth.js';

const router = Router();

router.use(requireAuth);
router.get('/dashboard', dashboard);
router.post('/chat', chat);
router.get('/revenue', revenue);
router.post('/summary/video', summarizeVideo);
router.post('/book', book);

export default router;
