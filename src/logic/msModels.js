/**
 * Models for Multiple Sclerosis Research
 */

export class Myelin {
  constructor(integrity = 1.0) {
    this.integrity = integrity; // 1.0 is healthy
  }
}

export class CentralNervousSystem {
  constructor() {
    this.myelin = new Myelin();
    this.isInflamed = false;
  }
}

export class ImmuneCell {
  constructor(type) {
    this.type = type; // 'T-Cell' or 'B-Cell'
    this.isActive = false;
  }
}

export class Nrf2Pathway {
  constructor() {
    this.isActivated = false;
  }

  activate() {
    this.isActivated = true;
    // Returns a symbolic factor representing the strength of neuroprotection
    return 0.7;
  }
}

export class DimethylFumarate {
  constructor() {
    this.name = "Dimethyl Fumarate";
    this.pathway = new Nrf2Pathway();
  }

  apply() {
    console.log(`Applying ${this.name}...`);
    return this.pathway.activate();
  }
}
