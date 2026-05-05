const { chromium } = require("playwright");

async function main() {
  const baseUrl = process.env.DASHBOARD_URL || "http://127.0.0.1:5173/";
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({
    viewport: { width: 1440, height: 1000 },
    deviceScaleFactor: 1,
  });

  await page.goto(baseUrl, { waitUntil: "networkidle" });

  const info = {
    title: await page.title(),
    h1: await page.locator("h1").textContent(),
    cells: await page.locator(".cell").count(),
    robots: await page.locator(".robotMarker").count(),
    routes: await page.locator("polyline").count(),
    overflow: await page.evaluate(() => ({
      x: document.documentElement.scrollWidth - document.documentElement.clientWidth,
      y: document.documentElement.scrollHeight - document.documentElement.clientHeight,
    })),
  };

  await browser.close();

  if (info.title !== "RAI Playfield Control Panel") {
    throw new Error(`Unexpected title: ${info.title}`);
  }
  if (info.h1 !== "Playfield Control Panel") {
    throw new Error(`Unexpected h1: ${info.h1}`);
  }
  if (info.cells !== 81) {
    throw new Error(`Expected 81 grid cells, found ${info.cells}`);
  }
  if (info.robots !== 14) {
    throw new Error(`Expected 14 robot markers, found ${info.robots}`);
  }
  if (info.routes !== 14) {
    throw new Error(`Expected 14 route overlays, found ${info.routes}`);
  }
  if (info.overflow.x !== 0) {
    throw new Error(`Unexpected horizontal overflow: ${info.overflow.x}`);
  }

  console.log(JSON.stringify(info, null, 2));
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
