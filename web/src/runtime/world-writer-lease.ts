// Same-origin tabs may render separately, but only ONE may advance/save
// the authoritative Core world. Web Locks are browser-managed; there is no
// clock-expiring, stealable localStorage lease or probabilistic token race.
// An unsupported browser fails closed rather than running two writers.
export interface WorldWriterLease {
  readonly active: boolean;
  release(): void;
}

export async function acquireWorldWriterLease(): Promise<WorldWriterLease> {
  const locks = typeof navigator === 'undefined' ? undefined : navigator.locks;
  if (!locks || typeof locks.request !== 'function') {
    throw Error('이 브라우저는 중복 월드 실행 방지 잠금을 지원하지 않습니다.');
  }
  return new Promise<WorldWriterLease>((resolve, reject) => {
    // The lock request Promise remains unresolved while we own the lease.
    // Never await request() itself: the callback deliberately awaits release.
    void locks.request('lifelens-active-core-world',
      { mode: 'exclusive', ifAvailable: true },
      async lock => {
        if (!lock) {
          reject(Error('다른 탭에서 LifeLens 월드를 실행 중입니다. 기존 탭을 닫고 다시 여세요.'));
          return;
        }
        let active = true;
        let unlock: (() => void) | undefined;
        const untilReleased = new Promise<void>(done => { unlock = done; });
        resolve({
          get active() { return active; },
          release() {
            if (!active) return;
            active = false;
            unlock?.();
          },
        });
        await untilReleased;
      }).catch(reject);
  });
}
