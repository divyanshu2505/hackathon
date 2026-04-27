# LifeOS AI — MERN Starter

This repository now contains a **MERN stack** foundation for the LifeOS AI super app concept.

## Stack
- **MongoDB** + Mongoose
- **Express.js** API server
- **React** frontend (Vite)
- **Node.js** runtime

## Features Implemented (PRD-aligned)
- JWT authentication (signup/login)
- Dashboard with activity metrics
- AI chat assistant endpoint (mock local AI response service)
- Revenue ideas endpoint
- Video summary endpoint
- Book generator endpoint
- Activity log persistence in MongoDB

## Run locally

### 1) Configure environment
Create `server/.env`:

```env
PORT=4000
MONGODB_URI=mongodb://127.0.0.1:27017/lifeos_ai
JWT_SECRET=replace_with_long_secret
```

### 2) Install dependencies
```bash
npm install
npm install --prefix server
npm install --prefix client
```

### 3) Start app
```bash
npm run dev
```
- Client: `http://localhost:5173`
- Server: `http://localhost:4000`

## Notes
- AI features currently use deterministic mock services inside `server/src/services/aiMockService.js`.
- This is ready for integrating local models such as Ollama + Whisper in the next iteration.
