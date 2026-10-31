"""Fit WADAHEX voice parameters to a real one-shot by minimising a multi-resolution
log-spectral distance (plus an envelope term). Usage:
    python3 fit.py out.json file1.wav [file2.wav ...]"""
import ctypes, json, sys, os, numpy as np, cma
from scipy.io import wavfile
from scipy.signal import resample_poly

HERE = os.path.dirname(os.path.abspath(__file__))
LIB = ctypes.CDLL(os.path.join(HERE, os.environ.get("VOICE_LIB", "libvoice.so")))
SR = 44100
DUR = 0.9
N = int(SR * DUR)

# name, lo, hi, log-scaled
PARAMS = [("tonePitch", 30, 1500, 1), ("bend", 0, 1, 0), ("bendTime", 20, 3000, 1), ("toneDecay", 20, 3000, 1),
          ("toneLvl", 0, 1, 0), ("shape", 0, 1, 0), ("noiseLvl", 0, 1, 0), ("noiseDecay", 5, 2000, 1),
          ("noiseColor", 0, 1, 0), ("noiseSweep", -1, 1, 0), ("cutoff", 100, 18000, 1), ("reso", 0, 0.95, 0),
          ("filtEnv", 0, 1, 0), ("filtDecay", 5, 2000, 1), ("click", 0, 1, 0), ("drive", 0, 1, 0)]
EXTRA = json.loads(os.environ.get("EXTRA_PARAMS", "[]"))   # model extensions append here: [name, lo, hi, log, index]


BOUNDS = {}   # per-sample overrides: index -> (lo, hi)


def spec():
    sp = [list(p) for p in PARAMS] + [list(e[:4]) for e in EXTRA]
    for i, (lo, hi) in BOUNDS.items(): sp[i][1], sp[i][2] = lo, hi
    return sp


def from_unit(u):
    out = []
    for (name, lo, hi, lg), x in zip(spec(), np.clip(u, 0, 1)):
        out.append(lo * (hi / lo) ** x if lg else lo + (hi - lo) * x)
    return np.array(out, np.float32)


def to_unit(vals):
    u = []
    for (name, lo, hi, lg), v in zip(spec(), vals):
        u.append(np.log(v / lo) / np.log(hi / lo) if lg else (v - lo) / (hi - lo))
    return np.clip(np.array(u), 0, 1)


def render(vals, tone_filt, seed=1):
    p = np.zeros(18 + 16, np.float32)
    p[:16] = vals[:16]; p[16] = tone_filt; p[17] = 1.0
    for e, v in zip(EXTRA, vals[16:]): p[e[4]] = v
    out = np.zeros(N, np.float32)
    LIB.render(p.ctypes.data_as(ctypes.POINTER(ctypes.c_float)), len(p), SR, N, ctypes.c_float(1.0), seed,
               out.ctypes.data_as(ctypes.POINTER(ctypes.c_float)))
    return out


def load(path):
    sr, x = wavfile.read(path)
    x = x.astype(np.float64)
    if x.ndim > 1: x = x.mean(1)
    if sr != SR: x = resample_poly(x, SR, sr)
    x /= np.max(np.abs(x)) + 1e-12
    on = int(np.argmax(np.abs(x) > 0.02))
    x = x[max(0, on - 8): max(0, on - 8) + N]
    return np.pad(x, (0, N - len(x))).astype(np.float32)


def band_matrix(nfft, nb=56, fmin=30.0, fmax=16000.0):
    f = np.fft.rfftfreq(nfft, 1 / SR); edges = np.geomspace(fmin, fmax, nb + 1)
    M = np.zeros((nb, len(f)), np.float32)
    for b in range(nb):
        lo, hi = edges[b], edges[b + 1]
        sel = (f >= lo) & (f < hi)
        if not sel.any(): sel[np.argmin(np.abs(f - (lo + hi) / 2))] = True
        M[b, sel] = 1.0
    return M

RES = [(2048, 256), (512, 64), (128, 32)]
MATS = {n: band_matrix(n, 56 if n >= 512 else 24) for n, _ in RES}
WINS = {n: np.hanning(n).astype(np.float32) for n, _ in RES}
WIN4K = np.hanning(4096).astype(np.float32)
LOWBINS = slice(int(30 * 4096 / SR), int(700 * 4096 / SR))


def end_of(x):
    """KB6 samples are hard-cut: compare only up to where the original stops."""
    nz = np.where(np.abs(x) > 2e-4)[0]
    return int(min(N, nz[-1] + 1)) if len(nz) else N


def features(x, L=N):
    x = x[:L] / (np.max(np.abs(x[:L])) + 1e-9)
    feats = []
    for n, h in RES:
        lim = L if n > 256 else min(L, int(SR * 0.08))  # short window only looks at the attack
        seg = x[:lim] if lim >= n else np.pad(x[:lim], (0, n - lim))
        fr = np.lib.stride_tricks.sliding_window_view(seg, n)[::h] * WINS[n]
        S = np.abs(np.fft.rfft(fr, axis=1)) ** 2
        E = S @ MATS[n].T
        feats.append(10 * np.log10(E + 1e-7))
    env = np.sqrt(np.convolve(x * x, np.ones(220) / 220, mode="same")[::110])
    feats.append(20 * np.log10(env + 1e-4))
    # fine low band (30-700 Hz, ~10 Hz bins) over the first 400 ms: locks the pitch sweep
    lim = min(L, int(SR * 0.4))
    fr = np.lib.stride_tricks.sliding_window_view(np.pad(x[:lim], (0, 4096)), 4096)[: lim : 512] * WIN4K
    S = np.abs(np.fft.rfft(fr, axis=1)[:, LOWBINS]) ** 2
    feats.append(10 * np.log10(S + 1e-7))
    return feats


def loss(fa, fb):
    w = [1.0, 0.7, 0.5, 0.6, 0.5]
    total = 0.0
    for k, (a, b) in enumerate(zip(fa, fb)):
        floor = a.max() - 70
        total += w[k] * np.mean(np.abs(np.maximum(a, floor) - np.maximum(b, floor)))
    return total


def pitch_track(x, L):
    n, h = 8192, 256
    fr = np.lib.stride_tricks.sliding_window_view(np.pad(x[:L], (0, n)), 2048)[: L : h] * np.hanning(2048)
    S = np.abs(np.fft.rfft(fr, n=n, axis=1)); f = np.fft.rfftfreq(n, 1 / SR); band = np.where((f > 25) & (f < 1500))[0]
    tt, ff = [], []
    for k, row in enumerate(S):
        r = row[band]
        if r.max() < S.max() * 0.02: break
        i = int(np.argmax(r))
        if 0 < i < len(r) - 1:
            a, b, c = np.log(r[i - 1: i + 2] + 1e-12); d = 0.5 * (a - c) / (a - 2 * b + c)
        else: d = 0.0
        tt.append((k * h + 1024) / SR); ff.append(f[band[0] + i] + d * (f[1] - f[0]))
    return np.array(tt), np.array(ff)


def pitch_prefit(x, L):
    """Fit f(t) = P * (r + (1 - r) * exp(-t / tau)), the plugin's analog bend curve (r = 2^-(4*bend))."""
    from scipy.optimize import least_squares
    t, f = pitch_track(x, L)
    if len(f) < 6: return 150.0, 0.8, 1000.0
    m = lambda q, t: q[0] * (q[1] + (1 - q[1]) * np.exp(-t / q[2]))
    q0 = [f[0] * 1.05, 0.2, 0.2]
    r = least_squares(lambda q: np.log2(m(q, t)) - np.log2(f), q0, bounds=([20, 1 / 16.5, 0.01], [3000, 0.999, 3]))
    P, rr, tau = r.x
    bend = float(np.clip(-np.log2(rr) / 4.0, 0.0, 1.0))
    return float(P), bend, float(tau * 6.9 * 1000.0)


def fit_one(path, evals=int(os.environ.get("EVALS", 2400)), seed=0):
    tgt = load(path); L = end_of(tgt); ft = features(tgt, L)
    P, bend, bt = pitch_prefit(tgt, L)
    bt = float(np.clip(bt, 25, 2900))
    BOUNDS.clear()
    BOUNDS[0] = (max(30, P * 0.88), min(1500, P * 1.12))
    BOUNDS[1] = (max(0.0, bend - 0.12), min(1.0, bend + 0.12))
    BOUNDS[2] = (max(20, bt * 0.7), min(3000, bt * 1.4))
    f0, tau = P, bt
    inits = [
        [P, bend, bt, 500, 0.9, 0.8, 0.4, 250, 0.6, 0.0, 3500, 0.15, 0.3, 120, 0.4, 0.2, 3000, 60],
        [P, bend, bt, 800, 0.7, 0.6, 0.7, 400, 0.85, 0.3, 1500, 0.4, 0.6, 80, 0.7, 0.1, 1000, 150],
    ]
    best = None
    for k, init in enumerate(inits):
        init = init[:16] + init[16:16 + len(EXTRA)]
        f = lambda u: loss(ft, features(render(from_unit(u), 0), L))
        es = cma.CMAEvolutionStrategy(to_unit(init), 0.22, {"bounds": [0, 1], "popsize": 16, "maxfevals": evals // len(inits),
                                                             "seed": seed + 1 + k, "verbose": -9})
        while not es.stop():
            X = es.ask(); es.tell(X, [f(np.array(x)) for x in X])
        if best is None or es.result.fbest < best[1]: best = (es.result.xbest, es.result.fbest, 0)
    vals = from_unit(best[0])
    names = [p[0] for p in PARAMS] + [e[0] for e in EXTRA]
    return {"file": os.path.basename(path), "loss": round(float(best[1]), 3), "toneFilt": best[2], "len": L,
            "P": round(f0, 1), "bendTime0": round(tau, 1), "bend0": round(bend, 3), "params": {n: round(float(v), 4) for n, v in zip(names, vals)}}


if __name__ == "__main__":
    from multiprocessing import Pool
    files = sys.argv[2:]
    if len(files) == 1 and files[0].startswith("@"):
        files = [l.strip() for l in open(files[0][1:]) if l.strip()]
    def safe(f):
        try: return fit_one(f)
        except Exception as e: return {"file": os.path.basename(f), "error": repr(e)}
    res = []
    with Pool(2) as pool:
        for r in pool.imap_unordered(safe, files):
            res.append(r); json.dump(res, open(sys.argv[1], "w"), indent=1)
            print(r["file"], r.get("loss", r.get("error")), flush=True)
