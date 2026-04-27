const products = [
  { id: 1, name: 'AeroSneak X1', desc: 'Self-lacing streetwear with adaptive sole suspension.', price: 189.0 },
  { id: 2, name: 'Halo Lens AR', desc: 'Featherweight AR visor for immersive urban navigation.', price: 329.0 },
  { id: 3, name: 'Pulse Desk Cube', desc: 'Mood-reactive smart workstation core with ambient sync.', price: 259.0 },
  { id: 4, name: 'NovaPod Mini', desc: 'Pocket projector that maps 3D media onto any surface.', price: 149.0 },
];

const productGrid = document.getElementById('productGrid');
const cartBtn = document.getElementById('cartBtn');
const cartPanel = document.getElementById('cartPanel');
const cartCount = document.getElementById('cartCount');
const cartItems = document.getElementById('cartItems');
const cartTotal = document.getElementById('cartTotal');
const shopNow = document.getElementById('shopNow');

const cart = [];

function money(value) {
  return value.toFixed(2);
}

function renderProducts() {
  productGrid.innerHTML = '';

  products.forEach((product) => {
    const card = document.createElement('article');
    card.className = 'product-card';
    card.innerHTML = `
      <h3>${product.name}</h3>
      <p>${product.desc}</p>
      <div class="price">$${money(product.price)}</div>
      <button class="ghost-btn" data-id="${product.id}">Add to Cart</button>
    `;
    productGrid.appendChild(card);
  });
}

function renderCart() {
  cartCount.textContent = cart.length;
  cartItems.innerHTML = cart.length
    ? cart.map((item) => `<li>${item.name} — $${money(item.price)}</li>`).join('')
    : '<li>Your cart is empty.</li>';

  const total = cart.reduce((sum, item) => sum + item.price, 0);
  cartTotal.textContent = money(total);
}

productGrid.addEventListener('click', (event) => {
  const btn = event.target.closest('button[data-id]');
  if (!btn) return;
  const product = products.find((p) => p.id === Number(btn.dataset.id));
  if (!product) return;

  cart.push(product);
  renderCart();
  cartPanel.classList.add('open');
});

cartBtn.addEventListener('click', () => {
  cartPanel.classList.toggle('open');
});

shopNow.addEventListener('click', () => {
  document.getElementById('catalog').scrollIntoView({ behavior: 'smooth', block: 'start' });
});

renderProducts();
renderCart();
