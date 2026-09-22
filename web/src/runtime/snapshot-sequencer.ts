export class SnapshotSequencer {
  private latestAccepted = -1;

  accept(sequence: number): boolean {
    if (!Number.isFinite(sequence)) return false;
    if (sequence <= this.latestAccepted) return false;
    this.latestAccepted = sequence;
    return true;
  }

  reset(): void {
    this.latestAccepted = -1;
  }

  get current(): number {
    return this.latestAccepted;
  }
}
