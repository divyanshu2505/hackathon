import bcrypt from 'bcryptjs';
import jwt from 'jsonwebtoken';
import { User } from '../models/User.js';

function tokenFor(user) {
  return jwt.sign({ sub: user._id, email: user.email, name: user.name }, process.env.JWT_SECRET, {
    expiresIn: '7d'
  });
}

export async function signup(req, res) {
  const { name, email, password, interests = [] } = req.body;

  if (!name || !email || !password) {
    return res.status(400).json({ message: 'name, email and password are required' });
  }

  const existing = await User.findOne({ email });
  if (existing) {
    return res.status(409).json({ message: 'Email already registered' });
  }

  const passwordHash = await bcrypt.hash(password, 10);
  const user = await User.create({ name, email, passwordHash, interests });

  return res.status(201).json({
    token: tokenFor(user),
    user: { id: user._id, name: user.name, email: user.email, interests: user.interests }
  });
}

export async function login(req, res) {
  const { email, password } = req.body;

  if (!email || !password) {
    return res.status(400).json({ message: 'email and password are required' });
  }

  const user = await User.findOne({ email });
  if (!user) {
    return res.status(401).json({ message: 'Invalid credentials' });
  }

  const match = await bcrypt.compare(password, user.passwordHash);
  if (!match) {
    return res.status(401).json({ message: 'Invalid credentials' });
  }

  return res.json({
    token: tokenFor(user),
    user: { id: user._id, name: user.name, email: user.email, interests: user.interests }
  });
}
