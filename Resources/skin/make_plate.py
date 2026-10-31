"""Builds the clean background plate for WADAHEX from the mockup: erases every live element
(knobs, labels, values, pads, LCD text, master knob, meter, trademark badge) and rebuilds the
panel surfaces so the plugin can draw the live controls on top."""
import sys
import numpy as np, cv2

src = cv2.imread("mock.png")
H, W = src.shape[:2]
out = src.copy()
lum = cv2.cvtColor(src, cv2.COLOR_BGR2GRAY).astype(np.int16)
debug = src.copy()


def region_mask(seed, tol=22):
    """Flood the dark panel surface from a seed, then fill the holes (knobs, text) inside it."""
    m = np.zeros((H + 2, W + 2), np.uint8)
    img = cv2.GaussianBlur(src, (3, 3), 0)
    cv2.floodFill(img.copy(), m, seed, (0, 0, 0), (tol,) * 3, (tol,) * 3,
                  4 | cv2.FLOODFILL_MASK_ONLY | (255 << 8))
    flood = m[1:-1, 1:-1] > 0
    cnts, _ = cv2.findContours(flood.astype(np.uint8), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
    full = np.zeros((H, W), np.uint8)
    cv2.drawContours(full, [max(cnts, key=cv2.contourArea)], -1, 255, -1)
    return flood, full > 0


def restore_screws(full, flood, region):
    """Holes that are small, roundish and near the region's corners are screws: put them back."""
    holes = (full & ~flood).astype(np.uint8)
    n, lab, st, cen = cv2.connectedComponentsWithStats(holes, 8)
    x0, y0, w, h = region
    corners = [(x0, y0), (x0 + w, y0), (x0, y0 + h), (x0 + w, y0 + h)]
    keep = np.zeros_like(full)
    for i in range(1, n):
        bw, bh = st[i, 2], st[i, 3]
        cx, cy = cen[i]
        if 6 <= bw <= 30 and 6 <= bh <= 30 and min(np.hypot(cx - a, cy - b) for a, b in corners) < 34:
            keep |= cv2.dilate((lab == i).astype(np.uint8), np.ones((5, 5), np.uint8)) > 0
    return keep


def fill_rows(mask, flood, src_mask=None, jitter=1.2, rng=np.random.default_rng(3), poly=None):
    """Replace mask pixels with the per-row median of the clean surface. poly=N fits a smooth
    degree-N curve over the rows so knob glow / text can't leave streaks."""
    ys, xs = np.where(mask)
    if len(ys) == 0: return
    sm = flood if src_mask is None else src_mask
    ks, vs, ws = [], [], []
    for y in range(ys.min(), ys.max() + 1):
        row = src[y][sm[y]]
        if len(row) < 5: continue
        ks.append(y); vs.append(np.median(row, axis=0)); ws.append(len(row))
    ks = np.array(ks); vs = np.array(vs, float); ws = np.sqrt(np.array(ws, float))
    yr = np.arange(ys.min(), ys.max() + 1)
    if poly:
        t = (ks - yr[0]) / max(1, yr[-1] - yr[0]); tr = (yr - yr[0]) / max(1, yr[-1] - yr[0])
        smooth = np.stack([np.polyval(np.polyfit(t, vs[:, c], poly, w=ws), tr) for c in range(3)], 1)
    else:
        smooth = np.stack([np.interp(yr, ks, vs[:, c]) for c in range(3)], 1)
        smooth = cv2.GaussianBlur(smooth.reshape(-1, 1, 3).astype(np.float32), (1, 5), 0).reshape(-1, 3)
    noise = rng.normal(0, jitter, (len(ys), 3))
    out[ys, xs] = np.clip(smooth[ys - yr[0]] + noise, 0, 255).astype(np.uint8)


# ---------------------------------------------------------------- control sections
sections = {"TONE": (1000, 240), "NOISE": (1460, 240), "CHIP": (940, 493), "FILTER": (1400, 493),
            "LFO": (900, 731), "OUTPUT": (1350, 731)}
ymin = {"TONE": 235, "NOISE": 235, "CHIP": 489, "FILTER": 489, "LFO": 728, "OUTPUT": 728}
rects = {}
for name, seed in sections.items():
    flood, full = region_mask(seed)
    full[: ymin[name], :] = False; flood[: ymin[name], :] = False   # keep title bar + stripe
    ys, xs = np.where(full)
    r = (xs.min(), ys.min(), xs.max() - xs.min(), ys.max() - ys.min())
    rects[name] = [int(v) for v in r]
    screws = restore_screws(full, flood, r)
    erase = full & ~screws
    # leave a 2 px rim of original pixels so the inner bevel stays intact
    erase = cv2.erode(erase.astype(np.uint8), np.ones((5, 5), np.uint8)) > 0
    b_, g_, r_ = src[..., 0].astype(np.int16), src[..., 1].astype(np.int16), src[..., 2].astype(np.int16)
    neutral = (np.abs(r_ - b_) < 9) & (lum > 10) & (lum < 42)
    fill_rows(erase, None, src_mask=flood & ~screws & neutral, poly=4)
    print(name, "seed", src[seed[1], seed[0]], "rect", rects[name])
    cv2.rectangle(debug, (r[0], r[1]), (r[0] + r[2], r[1] + r[3]), (0, 255, 0), 1)

# ---------------------------------------------------------------- pad panel (fixed interior rect)
px0, py0, px1, py1 = 35, 210, 718, 887
erase = np.zeros((H, W), np.uint8); erase[py0:py1, px0:px1] = 255
for sx, sy in ((40, 875), (708, 875)):
    cv2.circle(erase, (sx, sy), 15, 0, -1)
erase = erase > 0
rects["PADS"] = [px0, py0, px1 - px0, py1 - py0]
b, g, r = src[..., 0].astype(np.int16), src[..., 1].astype(np.int16), src[..., 2].astype(np.int16)
clean = (lum < 34) & (r - b < 10) & erase
fill_rows(erase, None, src_mask=clean, jitter=1.6, poly=4)
yy, xx = np.where(erase)
weave = (((xx + yy) // 3) % 6 == 0).astype(np.int16) * 3 - (((xx - yy) // 3) % 6 == 0).astype(np.int16) * 2
cx = (px0 + px1) / 2
vign = 1.0 - 0.25 * np.clip(np.abs(xx - cx) / ((px1 - px0) / 2), 0, 1) ** 2
out[yy, xx] = np.clip(out[yy, xx].astype(np.float32) * vign[:, None] + weave[:, None], 0, 255).astype(np.uint8)

# ---------------------------------------------------------------- LCD text ("SDS-V"), keep the window + triangle
lx0, ly0, lx1, ly1 = 896, 58, 1180, 98
box = src[ly0:ly1, lx0:lx1].astype(np.int16)
bg = (box[..., 0] < 140) & (box.sum(-1) < 260)          # not cyan text
m = np.zeros((H, W), bool); m[ly0:ly1, lx0:lx1] = True
sm = np.zeros((H, W), bool); sm[ly0:ly1, lx0:lx1] = bg
fill_rows(m, None, src_mask=sm, jitter=1.0)

# ---------------------------------------------------------------- master knob + LED meter
mx, my = 1511, 90
m = np.zeros((H, W), bool); m[47:134, 1452:1641] = True; m[24:47, 1548:1641] = True
sm = np.zeros((H, W), bool); sm[24:134, 1440:1451] = True
fill_rows(m, None, src_mask=sm, jitter=0.8)

cv2.imwrite("plate.png", out)
cv2.imwrite("/tmp/claude-0/plate_debug.png", debug)
import json; json.dump(rects, open("rects.json", "w"))
