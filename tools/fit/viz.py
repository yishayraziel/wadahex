import json, sys, numpy as np, matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
sys.argv=[sys.argv[0]]+sys.argv[1:]
from fit import *
res=json.load(open(sys.argv[1])); D='/mnt/user-data/uploads/[KB6]_Simmons_SDS5/'
fig,ax=plt.subplots(3,len(res),figsize=(5*len(res),11))
for j,r in enumerate(res):
    tgt=load(D+r['file']); vals=np.array(list(r['params'].values()),np.float32); y=render(vals,r['toneFilt']); y[r.get('len',N):]=0; y/=np.abs(y).max()
    for i,(x,t) in enumerate([(tgt,'orig'),(y,'fit')]):
        a=ax[i,j]; a.specgram(x+1e-9,NFFT=1024,Fs=SR,noverlap=900,cmap='magma',vmin=-120); a.set_ylim(0,8000); a.set_xlim(0,0.8); a.set_title(f"{r['file']} {t}")
    a=ax[2,j]
    for x,t in [(tgt,'orig'),(y,'fit')]:
        S=np.abs(np.fft.rfft(x[int(0.01*SR):int(0.3*SR)]*np.hanning(int(0.29*SR))))+1e-9
        f=np.fft.rfftfreq(int(0.29*SR),1/SR); a.semilogx(f,20*np.log10(S/S.max()+1e-6),lw=0.7,label=t)
    a.set_xlim(30,16000); a.set_ylim(-90,0); a.legend(); a.set_title('spectrum 10-300 ms')
plt.tight_layout(); plt.savefig(sys.argv[2],dpi=48)
