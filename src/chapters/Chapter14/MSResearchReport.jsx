import React, { useState, useEffect } from 'react';
import { MSResearchSimulation, PATHOLOGICAL_BASE, HEALTHY_TARGET } from '../../logic/msSimulation';
import { SEFIROT_MAPPING } from '../../logic/msSpiritual';
import { Heart, Shield, FlaskConical, Activity, CheckCircle2 } from 'lucide-react';

const MSResearchReport = () => {
  const [simulation, setSimulation] = useState(null);
  const [results, setResults] = useState(null);
  const [step, setStep] = useState(0); // 0: Start, 1: Damaged, 2: Cured

  useEffect(() => {
    setSimulation(new MSResearchSimulation());
  }, []);

  const handleSimulateDamage = () => {
    simulation.simulateDamage();
    setStep(1);
  };

  const handleApplyCure = () => {
    const res = simulation.applyHolisticCure();
    setResults(res);
    setStep(2);
  };

  const reset = () => {
    setSimulation(new MSResearchSimulation());
    setResults(null);
    setStep(0);
  };

  return (
    <div className="max-w-4xl mx-auto space-y-8">
      <section className="bg-white p-6 rounded-2xl border shadow-sm">
        <h2 className="text-2xl font-bold text-gray-900 mb-4 flex items-center">
          <Activity className="mr-2 text-blue-600" /> Отчет об исследовании: Лечение рассеянного склероза
        </h2>
        <p className="text-gray-600 mb-6">
          Этот отчет представляет собой программную модель комплексного подхода к лечению рассеянного склероза,
          объединяющую современную фармакологию (путь Nrf2) и древние концепции духовного баланса (Сфирот).
        </p>

        <div className="flex gap-4">
          {step === 0 && (
            <button
              onClick={handleSimulateDamage}
              className="bg-red-600 text-white px-6 py-2 rounded-lg hover:bg-red-700 transition-colors"
            >
              Симулировать патологию
            </button>
          )}
          {step === 1 && (
            <button
              onClick={handleApplyCure}
              className="bg-green-600 text-white px-6 py-2 rounded-lg hover:bg-green-700 transition-colors"
            >
              Применить комплексное лечение
            </button>
          )}
          {step === 2 && (
            <button
              onClick={reset}
              className="bg-gray-600 text-white px-6 py-2 rounded-lg hover:bg-gray-700 transition-colors"
            >
              Сбросить симуляцию
            </button>
          )}
        </div>
      </section>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
        {/* Status Panel */}
        <section className="bg-white p-6 rounded-2xl border shadow-sm">
          <h3 className="text-xl font-bold mb-4">Статус ЦНС</h3>
          <div className="space-y-4">
            <div>
              <div className="flex justify-between mb-1">
                <span className="text-sm font-medium text-gray-700">Целостность миелина</span>
                <span className="text-sm font-medium text-gray-700">
                  {simulation ? Math.round(simulation.cns.myelin.integrity * 100) : 100}%
                </span>
              </div>
              <div className="w-full bg-gray-200 rounded-full h-2.5">
                <div
                  className={`h-2.5 rounded-full transition-all duration-500 ${
                    (simulation?.cns.myelin.integrity || 1) < 0.6 ? 'bg-red-600' : 'bg-green-600'
                  }`}
                  style={{ width: `${(simulation?.cns.myelin.integrity || 1) * 100}%` }}
                ></div>
              </div>
            </div>

            <div className="flex items-center gap-2">
              <span className="text-sm font-medium">Воспаление:</span>
              {simulation?.cns.isInflamed ? (
                <span className="text-red-600 font-bold px-2 py-0.5 bg-red-50 rounded border border-red-200 text-xs">Активно</span>
              ) : (
                <span className="text-green-600 font-bold px-2 py-0.5 bg-green-50 rounded border border-green-200 text-xs">Отсутствует</span>
              )}
            </div>
          </div>

          {results && (
            <div className="mt-8 pt-6 border-t space-y-4">
              <h4 className="font-bold flex items-center">
                <FlaskConical size={18} className="mr-2 text-purple-600" /> Рецепт лечения
              </h4>
              <ul className="text-sm space-y-2 text-gray-600">
                <li className="flex items-center">
                  <CheckCircle2 size={14} className="mr-2 text-green-500" />
                  Химический фактор (Nrf2): +{Math.round(results.chemicalFactor * 100)}%
                </li>
                <li className="flex items-center">
                  <CheckCircle2 size={14} className="mr-2 text-green-500" />
                  Духовный фактор (Сфирот): +{Math.round(results.spiritualFactor * 100)}%
                </li>
              </ul>
              <div className="p-4 bg-blue-50 rounded-xl border border-blue-100">
                <p className="text-sm text-blue-800 font-medium italic">
                  "Гармония между химией тела и балансом души ведет к полному восстановлению."
                </p>
              </div>
            </div>
          )}
        </section>

        {/* Spiritual Map Panel */}
        <section className="bg-white p-6 rounded-2xl border shadow-sm overflow-hidden">
          <h3 className="text-xl font-bold mb-4 flex items-center">
            <Heart size={20} className="mr-2 text-red-500" /> Карта соответствия Сфирот
          </h3>
          <div className="max-h-[400px] overflow-y-auto pr-2 space-y-2">
            {SEFIROT_MAPPING.map((s, idx) => (
              <div key={idx} className="flex items-center justify-between p-3 bg-gray-50 rounded-lg border text-sm">
                <div>
                  <div className="font-bold text-blue-800">{s.name} ({s.translation})</div>
                  <div className="text-xs text-gray-500">{s.anatomy} — {s.aspect}</div>
                </div>
                {step === 2 && (
                  <Shield size={16} className="text-green-600" />
                )}
              </div>
            ))}
          </div>
        </section>
      </div>
    </div>
  );
};

export default MSResearchReport;
