// Kremų Namai – SPA logika.
// Produktai hardcoded žemiau. Nuotraukos: data/img/p{id}.jpg (žr. img/README.txt);
// jei nuotraukos nėra – rodoma generuojama SVG iliustracija.
const IMG_EXT = 'jpg';
const P = [
 {id:1,n:"Rožių veido kremas",c:"Veidui",p:14.90,col:"#e8b4c8",d:"Maitinamasis veido kremas su damaskinių rožių aliejumi. Tinka sausai ir normaliai odai.",i:["Rožių aliejus","Taukmedžio sviestas","Vitaminas E","Bičių vaškas"],feat:1},
 {id:2,n:"Levandų naktinis kremas",c:"Veidui",p:16.50,col:"#b8a8d8",d:"Raminantis naktinis kremas su levandų eteriniu aliejumi. Atstato odą miego metu.",i:["Levandų aliejus","Avokadų aliejus","Skvalanas","Ramunėlių ekstraktas"],feat:1},
 {id:3,n:"Medaus rankų kremas",c:"Rankoms",p:8.90,col:"#e8c890",d:"Intensyviai drėkinantis rankų kremas su natūraliu medumi ir propoliu.",i:["Medus","Propolis","Alyvuogių aliejus","Glicerinas"],feat:1},
 {id:4,n:"Alavijo kūno losjonas",c:"Kūnui",p:12.40,col:"#a8d8b0",d:"Lengvas, greitai susigeriantis kūno losjonas su alavijo sultimis. Gaivina ir drėkina.",i:["Alavijo sultys","Kokosų aliejus","Agurkų ekstraktas"],feat:0},
 {id:5,n:"Kalendulos kremas jautriai odai",c:"Veidui",p:15.90,col:"#f0b878",d:"Švelnus kremas su medetkų ekstraktu. Ramina sudirgusią ir jautrią odą.",i:["Medetkų ekstraktas","Jojobos aliejus","Pantenolis"],feat:0},
 {id:6,n:"Šaltalankių kūno sviestas",c:"Kūnui",p:13.80,col:"#f0a060",d:"Sodrus kūno sviestas su šaltalankių aliejumi. Ypač sausai odai.",i:["Šaltalankių aliejus","Kakavos sviestas","Taukmedžio sviestas"],feat:0},
 {id:7,n:"Mėtų pėdų kremas",c:"Pėdoms",p:9.50,col:"#90d0c0",d:"Gaivinantis pėdų kremas su pipirmėčių aliejumi ir arbatmedžiu. Minkština sudiržusią odą.",i:["Pipirmėčių aliejus","Arbatmedžio aliejus","Karbamidas","Šalavijas"],feat:0},
 {id:8,n:"Ramunėlių kremas vaikams",c:"Vaikams",p:11.90,col:"#f0d8a0",d:"Itin švelnus kremas kūdikių ir vaikų odai su ramunėlių ekstraktu. Be kvapiklių.",i:["Ramunėlių ekstraktas","Migdolų aliejus","Cinko oksidas"],feat:0},
 {id:9,n:"Paakių kremas su kofeinu",c:"Veidui",p:17.90,col:"#c8b8a8",d:"Gaivinantis paakių kremas, mažinantis paburkimus ir tamsius ratilus.",i:["Kofeinas","Žaliosios arbatos ekstraktas","Hialurono rūgštis"],feat:0},
 {id:10,n:"Apsauginis žiemos kremas",c:"Veidui",p:13.50,col:"#a8c8e0",d:"Apsauginis kremas nuo šalčio ir vėjo. Sukuria apsauginį barjerą odai.",i:["Bičių vaškas","Lanolinas","Saulėgrąžų aliejus"],feat:0}
];

// --- krepšelis iš localStorage (su validacija) ---
let cart = {};
try {
  const saved = JSON.parse(localStorage.getItem('cart') || '{}');
  for (const id in saved) {
    const q = saved[id];
    if (P.some(p => p.id == id) && Number.isFinite(q) && q > 0) cart[id] = Math.floor(q);
  }
} catch(e) {}

// --- paveikslėliai: nuotrauka img/p{id}.jpg, fallback – SVG ---
function svgPic(p) {
  return `<svg viewBox="0 0 200 150" preserveAspectRatio="xMidYMid slice" aria-hidden="true">
    <rect width="200" height="150" fill="#fdf0f5"/>
    <ellipse cx="100" cy="128" rx="46" ry="6" fill="rgba(0,0,0,.07)"/>
    <rect x="60" y="55" width="80" height="72" rx="10" fill="${p.col}"/>
    <rect x="60" y="42" width="80" height="18" rx="6" fill="${shade(p.col)}"/>
    <path d="M84 88c5-8 12-11 16-9 4-2 11 1 16 9 4 6-3 14-16 20-13-6-20-14-16-20z" fill="#fff" opacity=".9"/>
    <circle cx="162" cy="38" r="9" fill="${p.col}" opacity=".4"/>
    <circle cx="36" cy="98" r="6" fill="${p.col}" opacity=".4"/>
  </svg>`;
}
function pic(p, cls) {
  return `<div class="p-img ${cls||''}" title="${p.n}">${svgPic(p)}
    <img src="img/p${p.id}.${IMG_EXT}" alt="${p.n}" loading="lazy"
      onload="this.classList.add('ok')" onerror="this.remove()"></div>`;
}
function shade(hex) {
  const n = parseInt(hex.slice(1), 16);
  const f = x => Math.max(0, x - 40);
  return '#' + [f(n>>16), f((n>>8)&255), f(n&255)].map(x=>x.toString(16).padStart(2,'0')).join('');
}
function eur(v){ return v.toFixed(2).replace('.',',') + ' €'; }

// --- toast pranešimai ---
let toastT;
function toast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  clearTimeout(toastT);
  toastT = setTimeout(() => t.classList.remove('show'), 2200);
}

// --- navigacija ---
function show(page) {
  document.querySelectorAll('.page').forEach(s => s.hidden = true);
  const el = document.getElementById('page-' + page);
  if (!el) return;
  el.hidden = false;
  el.classList.remove('fade'); void el.offsetWidth; el.classList.add('fade');
  document.querySelectorAll('nav a').forEach(a =>
    a.classList.toggle('act', a.dataset.p === page));
  document.getElementById('nav').classList.remove('open');
  if (page === 'cart') renderCart();
  window.scrollTo(0, 0);
}
function toggleNav() {
  document.getElementById('nav').classList.toggle('open');
}

// --- produktų kortelės ---
function cardHTML(p) {
  return `<div class="card p-card" onclick="openProduct(${p.id})">
    ${pic(p)}
    <div class="p-body">
      <div class="p-cat">${p.c}</div>
      <h3>${p.n}</h3>
      <div class="p-row">
        <div class="p-price">${eur(p.p)}</div>
        <button class="btn btn-sm" onclick="event.stopPropagation();add(${p.id})">Į krepšelį</button>
      </div>
    </div>
  </div>`;
}

let curCat = 'Visi';
function renderShop() {
  const cats = ['Visi', ...new Set(P.map(p => p.c))];
  document.getElementById('filters').innerHTML = cats.map(c =>
    `<button class="${c===curCat?'on':''}" onclick="curCat='${c}';renderShop()">${c}</button>`).join('');
  const list = curCat === 'Visi' ? P : P.filter(p => p.c === curCat);
  document.getElementById('products').innerHTML = list.map(cardHTML).join('');
}

function openProduct(id) {
  const p = P.find(x => x.id === id);
  if (!p) return;
  document.getElementById('productDetail').innerHTML = `
    <button class="back" onclick="show('shop')">← Grįžti į parduotuvę</button>
    <div class="detail">
      ${pic(p, 'detail-img')}
      <div class="detail-txt">
        <div class="p-cat">${p.c}</div>
        <h2>${p.n}</h2>
        <div class="p-price">${eur(p.p)}</div>
        <p>${p.d}</p>
        <b>Sudėtis:</b>
        <ul>${p.i.map(x=>`<li>${x}</li>`).join('')}</ul>
        <div class="qty">
          <button onclick="qv(-1)" aria-label="Mažiau">−</button><span id="qv">1</span><button onclick="qv(1)" aria-label="Daugiau">+</button>
        </div><br>
        <button class="btn" onclick="add(${p.id}, +document.getElementById('qv').textContent)">Į krepšelį</button>
      </div>
    </div>`;
  show('product');
}
function qv(d) {
  const el = document.getElementById('qv');
  el.textContent = Math.max(1, +el.textContent + d);
}

// --- krepšelis ---
function add(id, q) {
  if (!P.some(p => p.id === id)) return;
  cart[id] = (cart[id] || 0) + (q || 1);
  saveCart();
  toast('✓ Pridėta į krepšelį');
  const b = document.getElementById('cartCount');
  b.classList.remove('pop'); void b.offsetWidth; b.classList.add('pop');
}
function saveCart() {
  localStorage.setItem('cart', JSON.stringify(cart));
  const n = Object.values(cart).reduce((a,b)=>a+b, 0);
  document.getElementById('cartCount').textContent = n;
}
function renderCart() {
  const box = document.getElementById('cartItems');
  const co = document.getElementById('checkoutBox');
  document.getElementById('orderMsg').hidden = true;
  const ids = Object.keys(cart);
  if (!ids.length) {
    box.innerHTML = `<div class="empty">
      <div class="empty-ico">🛒</div>
      <p>Krepšelis tuščias</p>
      <button class="btn" onclick="show('shop')">Eiti į parduotuvę</button>
    </div>`;
    co.hidden = true;
    return;
  }
  let total = 0;
  box.innerHTML = ids.map(id => {
    const p = P.find(x => x.id == id), q = cart[id];
    total += p.p * q;
    return `<div class="cart-row">${pic(p, 'cart-img')}
      <div class="grow"><b>${p.n}</b><br><span class="muted">${eur(p.p)} × ${q}</span></div>
      <div class="qty"><button onclick="cq(${id},-1)" aria-label="Mažiau">−</button><span>${q}</span><button onclick="cq(${id},1)" aria-label="Daugiau">+</button></div>
      <b class="row-sum">${eur(p.p*q)}</b>
      <button class="x" onclick="delete cart[${id}];saveCart();renderCart()" aria-label="Pašalinti">✕</button>
    </div>`;
  }).join('') + `<div class="cart-total">Iš viso: <b>${eur(total)}</b></div>`;
  co.hidden = false;
}
function cq(id, d) {
  if (!(id in cart)) return;
  cart[id] += d;
  if (cart[id] <= 0) delete cart[id];
  saveCart(); renderCart();
}

// --- užsakymas -> ESP32 ---
function sendOrder(e) {
  e.preventDefault();
  const f = e.target;
  const btn = f.querySelector('button[type=submit]');
  if (btn.disabled) return;
  btn.disabled = true; btn.textContent = 'Siunčiama…';
  const items = Object.keys(cart).map(id => {
    const p = P.find(x => x.id == id);
    return { id: p.id, n: p.n, q: cart[id], p: p.p };
  });
  const total = items.reduce((a,i)=>a+i.p*i.q, 0);
  // Paruoštas tekstas Telegram žinutei (ESP32 jį tiesiog persiunčia)
  const tg = '🛒 NAUJAS UŽSAKYMAS\n'
    + items.map(i => `• ${i.n} × ${i.q}`).join('\n')
    + `\n💰 Iš viso: ${total.toFixed(2)} €`
    + `\n👤 ${f.name.value}\n📞 ${f.phone.value}`
    + (f.note.value ? `\n📝 ${f.note.value}` : '');
  const order = { name: f.name.value, phone: f.phone.value, note: f.note.value, items, total: +total.toFixed(2), tg };
  fetch('/api/order', {
    method: 'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify(order)
  }).then(r => r.ok ? r.json() : Promise.reject())
    .then(j => done('✅ Užsakymas #' + j.id + ' priimtas! Susisieksime telefonu ' + order.phone + '.'))
    .catch(() => done('✅ Užsakymas užregistruotas vietoje. Parodykite jį kasoje arba paskambinkite +370 600 00000.'));
  function done(msg) {
    cart = {}; saveCart(); f.reset();
    btn.disabled = false; btn.textContent = 'Pateikti užsakymą';
    document.getElementById('cartItems').innerHTML = '';
    document.getElementById('checkoutBox').hidden = true;
    const m = document.getElementById('orderMsg');
    m.textContent = msg; m.hidden = false;
  }
}

// --- init ---
document.getElementById('featured').innerHTML = P.filter(p=>p.feat).map(cardHTML).join('');
renderShop();
saveCart();
