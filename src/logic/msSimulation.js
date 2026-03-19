import { CentralNervousSystem, DimethylFumarate } from './msModels';
import { getAllSefirot } from './msSpiritual';

export const PATHOLOGICAL_BASE = 0.5;
export const HEALTHY_TARGET = 1.0;

export class MSResearchSimulation {
  constructor() {
    this.cns = new CentralNervousSystem();
    this.medicine = new DimethylFumarate();
    this.sefirot = getAllSefirot();
  }

  simulateDamage() {
    console.log("Simulating MS Pathophysiology (Autoimmune attack)...");
    this.cns.myelin.integrity = PATHOLOGICAL_BASE;
    this.cns.isInflamed = true;
    return this.cns.myelin.integrity;
  }

  calculateSpiritualFactor() {
    console.log("Balancing Sefirot (Spiritual Alignment)...");
    let spiritualFactor = 0;
    this.sefirot.forEach(s => {
      spiritualFactor += s.balance();
    });
    // For 10 sefirot (Keter or Daat usually excluded in some counts, but here we use 10 active),
    // it gives 10 * 0.03 = 0.3
    // Note: We have 11 in our mapping. 10 * 0.03 = 0.3. If all 10 are balanced.
    // Let's ensure it adds up correctly for the simulation.
    return spiritualFactor;
  }

  applyHolisticCure() {
    console.log("Applying Holistic Cure (Chemistry + Spirituality)...");

    // 1. Chemical factor from Nrf2 activation (Dimethyl Fumarate)
    const chemicalFactor = this.medicine.apply(); // returns 0.7

    // 2. Spiritual factor from Sefirot alignment
    const spiritualFactor = this.calculateSpiritualFactor();

    // 3. Holistic result
    // Starting from 0.5 (PATHOLOGICAL_BASE)
    // + 0.5 (needed to reach 1.0)
    // Here we have 0.7 (chemical) and ~0.3 (spiritual)
    // To be precise, if we use 10 sefirot for the factor:
    // Integrity = base + chemicalFactor + spiritualFactor
    // We will cap it at 1.0

    this.cns.myelin.integrity = Math.min(
      HEALTHY_TARGET,
      this.cns.myelin.integrity + chemicalFactor + spiritualFactor
    );

    if (this.cns.myelin.integrity >= HEALTHY_TARGET) {
      this.cns.isInflamed = false;
      console.log("Cure Successful: CNS Myelin Integrity restored to 100%.");
    }

    return {
      finalIntegrity: this.cns.myelin.integrity,
      chemicalFactor,
      spiritualFactor,
      isInflamed: this.cns.isInflamed
    };
  }
}
