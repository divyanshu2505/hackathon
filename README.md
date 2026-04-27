# LifeOS AI — MERN Starter

This repository contains a **MERN stack** foundation for the LifeOS AI super app concept.

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

## Local Development

### 1) Configure environment
Create `server/.env`:

```env
PORT=4000
MONGODB_URI=mongodb://127.0.0.1:27017/lifeos_ai
JWT_SECRET=replace_with_long_secret
```

Optionally create `client/.env` from `client/.env.example` if API is running on a different host:

```env
VITE_API_BASE_URL=http://localhost:4000
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

---

## Vercel Deployment (Frontend + API)

Is repo ko Vercel par deploy karne ke liye configuration add ki gayi hai (`vercel.json` + `api/index.js`).

### Steps
1. Push this repo to GitHub.
2. In Vercel, click **Add New Project** and import the repo.
3. Set these Environment Variables in Vercel Project Settings:
   - `MONGODB_URI` (MongoDB Atlas URI)
   - `JWT_SECRET` (strong random secret)
   - `VITE_API_BASE_URL` (optional; leave blank for same-domain API)
4. Deploy.

### Deployment behavior
- React app is built from `client/`.
- All `/api/*` requests are routed to serverless function `api/index.js`, which uses Express app from `server/src/app.js`.

## Notes
- AI features currently use deterministic mock services in `server/src/services/aiMockService.js`.
- Ready for next iteration: Ollama + Whisper integration.
