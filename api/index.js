let cachedHandler;
let cachedDbPromise;

module.exports = async (req, res) => {
  if (!cachedHandler) {
    const [{ default: app }, { connectDB }] = await Promise.all([
      import('../server/src/app.js'),
      import('../server/src/config/db.js')
    ]);

    if (!cachedDbPromise) {
      cachedDbPromise = connectDB(process.env.MONGODB_URI);
    }
    await cachedDbPromise;
    cachedHandler = app;
  }

  return cachedHandler(req, res);
};
