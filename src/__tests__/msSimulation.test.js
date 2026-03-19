import { describe, it, expect } from 'vitest';
import { MSResearchSimulation, PATHOLOGICAL_BASE, HEALTHY_TARGET } from '../logic/msSimulation';

describe('MSResearchSimulation', () => {
  it('should initialize with healthy CNS', () => {
    const sim = new MSResearchSimulation();
    expect(sim.cns.myelin.integrity).toBe(1.0);
    expect(sim.cns.isInflamed).toBe(false);
  });

  it('should simulate damage to pathological level', () => {
    const sim = new MSResearchSimulation();
    const integrity = sim.simulateDamage();
    expect(integrity).toBe(PATHOLOGICAL_BASE);
    expect(sim.cns.isInflamed).toBe(true);
  });

  it('should calculate spiritual factor based on balanced sefirot', () => {
    const sim = new MSResearchSimulation();
    const factor = sim.calculateSpiritualFactor();
    // 11 sefirot * 0.03 = 0.33
    expect(factor).toBeCloseTo(0.33, 2);
  });

  it('should apply holistic cure and restore integrity to 100%', () => {
    const sim = new MSResearchSimulation();
    sim.simulateDamage(); // integrity = 0.5

    const result = sim.applyHolisticCure();

    // Pathological 0.5 + Chemical 0.7 + Spiritual 0.33 = 1.53, capped at 1.0
    expect(result.finalIntegrity).toBe(HEALTHY_TARGET);
    expect(result.isInflamed).toBe(false);
    expect(result.chemicalFactor).toBe(0.7);
    expect(result.spiritualFactor).toBeCloseTo(0.33, 2);
  });
});
