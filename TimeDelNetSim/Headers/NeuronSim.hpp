#ifndef NEURONSIM_HPP
#define NEURONSIM_HPP

#include "Network.hpp"

#include <ExternalInputCurrent/IExtCode.hpp>

#include <MexMemoryInterfacing/Headers/MexMem.hpp>
#include <MexMemoryInterfacing/Headers/GenericMexIO.hpp>
#include <MexMemoryInterfacing/Headers/LambdaToFunction.hpp>
#include <MexMemoryInterfacing/Headers/FlatVectTree/FlatVectTree.hpp>
#include <RandomNumGen/Headers/FiltRandomTBB.hpp>

#include <utility>
#include <stdint.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <tbb/atomic.h>
#include <tbb/parallel_for.h>

#include <emmintrin.h>
#include <smmintrin.h>

#define DEFAULT_STORAGE_STEP 500
#define DEFAULT_STATUS_DISPLAY_STEP 400
using namespace std;

struct OutOps{
	enum {
		V_REQ               = (1 <<  0), 
		I_IN_1_REQ          = (1 <<  1), 
		I_IN_2_REQ          = (1 <<  2), 
		I_IN_REQ            = (1 <<  3), 
		WEIGHT_DERIV_REQ    = (1 <<  4), 
		TIME_REQ            = (1 <<  5), 
		U_REQ               = (1 <<  6), 
		WEIGHT_REQ          = (1 <<  7), 
		CURRENT_QINDS_REQ   = (1 <<  8), 
		SPIKE_QUEUE_REQ     = (1 <<  9), 
		LASTSPIKED_NEU_REQ  = (1 << 10), 
		LASTSPIKED_SYN_REQ  = (1 << 11), 
		I_TOT_REQ           = (1 << 12), 
		SPIKE_LIST_REQ      = (1 << 13), 
		INITIAL_STATE_REQ   = (1 << 14), 
		FINAL_STATE_REQ     = (1 << 15)
	};
};

typedef vector<tbb::atomic<int64_t>, tbb::zero_allocator<int64_t> > atomicLongVect;
typedef vector<tbb::atomic<int32_t>, tbb::zero_allocator<int32_t> > atomicIntVect;

// Incomplete declarations
struct InputArgs;
struct StateVarsOutStruct;
struct SingleStateStruct;
struct OutputVarsStruct;
struct InternalVars;

struct CurrentUpdate
{
	InternalVars &IntVars;

	CurrentUpdate(InternalVars &IntVars_) :
		IntVars(IntVars_){};
	void operator() (const tbb::blocked_range<int32_t*> &BlockedRange) const;
};
struct NeuronSimulate{
	InternalVars &IntVars;

	NeuronSimulate(
		InternalVars &IntVars_
		) :
		IntVars(IntVars_)
	{};
	void operator() (tbb::blocked_range<int> &Range) const;
};

struct CurrentAttenuate{
	InternalVars &IntVars;

	CurrentAttenuate(
		InternalVars &IntVars_) :
		IntVars(IntVars_){}

	void operator() (tbb::blocked_range<int> &Range) const; 
};

struct SingleStateStruct{
	MexVector<float> Weight;
	MexVector<float> V;
	MexVector<float> U;
	MexVector<float> Iin1;
	MexVector<float> Iin2;
	MexVector<float> WeightDeriv;
	
	// IextInterface state variable component
	IExtInterface::SingleStateStruct IextInterface;

	FlatVectTree<int32_t> SpikeQueue;
	MexVector<int32_t> LSTNeuron;
	MexVector<int32_t> LSTSyn;
	int32_t Time;
	int32_t CurrentQIndex;

	SingleStateStruct() :
		Weight(),
		V(),
		U(),
		Iin1(),
		Iin2(),
		WeightDeriv(),
		IextInterface(),
		SpikeQueue(),
		LSTNeuron(),
		LSTSyn() {}

	void initialize(const InternalVars &);
};

struct InputArgs{
	MexVector<int32_t>   NStart;
	MexVector<int32_t>   NEnd;
	MexVector<float> Weight;
	MexVector<float> Delay;
	
	MexVector<float> a;
	MexVector<float> b;
	MexVector<float> c;
	MexVector<float> d;

	MexVector<int32_t> InterestingSyns;

	// IExt Interface Input Variables
	IExtInterface::InputVarsStruct IextInterface;

	// Initial State
	SingleStateStruct InitialState;

	// Compulsory Simulation Parameters
	size_t onemsbyTstep;
	size_t NoOfms;
	size_t DelayRange;

	// Optional Simulation Parameters
	size_t OutputControl;
	size_t StorageStepSize;
	size_t StatusDisplayInterval;
	MexVector<char> OutputControlString;

	// Optional Simulation Algorithm Parameters
	float I0;
	float CurrentDecayFactor1, CurrentDecayFactor2;

	InputArgs() :
		NStart(),
		NEnd(),
		Weight(),
		Delay(),
		a(),
		b(),
		c(),
		d(),
		InterestingSyns(),
		IextInterface(),
		OutputControlString(),
		InitialState() {}
};

struct InternalVars{

	// Compulsory Simulation Parameters
	size_t onemsbyTstep;
	size_t NoOfms;
	size_t DelayRange;
	
	// Other Constants
	size_t N;
	size_t M;
	size_t i;       //This is the most important loop index that is definitely a state variable
	                // and plays a crucial role in deciding the index into which the output must be performed
	size_t Time;    // must be initialized befor beta
	size_t nSteps;  // Included because, it is changed from its default value in case of abortion
	uint64_t NoOfSpikes;

	// Optional Simulation Parameters
	MexVector<char> OutputControlString;
	size_t OutputControl;
	size_t StorageStepSize;
	const size_t StatusDisplayInterval;

	// Optional Simulation Algorithm Parameters
	const float I0;
	const float CurrentDecayFactor1, CurrentDecayFactor2;

	// Scalar State Variables
	// Time is defined earlier for reasons of initialization sequence
	size_t CurrentQIndex;

	// Parameters that control C=Spike Storage Buffering
	size_t CacheBuffering;

	MexVector<Synapse> Network;
	MexVector<Neuron> Neurons;
	MexVector<int32_t> &InterestingSyns;
	MexVector<float> &V;
	MexVector<float> &U;
	atomicLongVect Iin1;
	atomicLongVect Iin2;
	MexVector<float> WeightDeriv;
	IExtInterface::InternalVarsStruct IextInterface;

	MexVector<MexVector<int32_t> > SpikeQueue;
	MexVector<int32_t> &LSTNeuron;
	MexVector<int32_t> &LSTSyn;

	MexVector<size_t> AuxArray;						    // Auxillary Array that is an indirection between Network
													    // and an array sorted lexicographically by (NEnd, NStart)
	MexVector<size_t> PreSynNeuronSectionBeg;	        // PreSynNeuronSectionBeg[j] Maintains the list of the 
														// index of the first synapse in Network with NStart = j+1
	MexVector<size_t> PreSynNeuronSectionEnd;	        // PostSynNeuronSectionEnd[j] Maintains the list of the 
														// indices one greater than index of the last synapse in 
														// Network with NStart = j+1

	MexVector<size_t> PostSynNeuronSectionBeg;	        // PostSynNeuronSectionBeg[j] Maintains the list of the 
														// index of the first synapse in AuxArray with NEnd = j+1
	MexVector<size_t> PostSynNeuronSectionEnd;	        // PostSynNeuronSectionEnd[j] Maintains the list of the 
														// indices one greater than index of the last synapse in 
														// AuxArray with NEnd = j+1

	// These vectors are instrumental in the Cache aligned 
	// implementation of Spike Storage
	MexVector<__m128i> BinningBuffer;	//each element is 16 bytes
	MexVector<int32_t> BufferInsertIndex;
	MexVector<int32_t> AddressOffset;

	InternalVars(InputArgs &IArgs) :
		N                     (IArgs.a.size()),
		M                     (IArgs.NStart.size()),
		i                     (0),
		Time                  (IArgs.InitialState.Time),
		nSteps                (onemsbyTstep*NoOfms),
		NoOfSpikes            (0),
		CurrentQIndex         (IArgs.InitialState.CurrentQIndex),
		OutputControl         (IArgs.OutputControl),
		OutputControlString   (IArgs.OutputControlString),
		StorageStepSize       (IArgs.StorageStepSize),
		StatusDisplayInterval (IArgs.StatusDisplayInterval),
		Network               (N),
		Neurons               (M),
		InterestingSyns       (IArgs.InterestingSyns),
		V                     (IArgs.InitialState.V),
		U                     (IArgs.InitialState.U),
		Iin1(N),	// Iin is defined separately as an atomic vect.
		Iin2(N),
		WeightDeriv           (IArgs.InitialState.WeightDeriv),
		IextInterface         (),
		SpikeQueue            (),
		LSTNeuron             (IArgs.InitialState.LSTNeuron),
		LSTSyn                (IArgs.InitialState.LSTSyn),
		AuxArray                (M),
		PreSynNeuronSectionBeg  (N, -1),
		PreSynNeuronSectionEnd  (N, -1),
		PostSynNeuronSectionBeg (N, -1),
		PostSynNeuronSectionEnd (N, -1),

		BinningBuffer     (CacheBuffering*onemsbyTstep*DelayRange / 4),
		BufferInsertIndex (onemsbyTstep*DelayRange, 0),
		AddressOffset     (onemsbyTstep*DelayRange, 0),

		onemsbyTstep       (IArgs.onemsbyTstep),
		NoOfms             (IArgs.NoOfms),
		DelayRange         (IArgs.DelayRange),
		CacheBuffering     (128),
		I0                 (IArgs.I0),
		CurrentDecayFactor1(IArgs.CurrentDecayFactor1),
		CurrentDecayFactor2(IArgs.CurrentDecayFactor2){
		// Setting up Network and Neurons
		Network.resize(M);
		MexTransform(IArgs.NStart.begin(), IArgs.NStart.end(), Network.begin(), FFL([ ](Synapse &Syn, int32_t &NStart)->void{Syn.NStart        = NStart       ; }));
		MexTransform(IArgs.NEnd  .begin(), IArgs.NEnd  .end(), Network.begin(), FFL([ ](Synapse &Syn, int32_t &NEnd  )->void{Syn.NEnd          = NEnd         ; }));
		MexTransform(IArgs.Weight.begin(), IArgs.Weight.end(), Network.begin(), FFL([ ](Synapse &Syn, float   &Weight)->void{Syn.Weight        = Weight       ; }));
		MexTransform(IArgs.Delay .begin(), IArgs.Delay .end(), Network.begin(), FFL([&](Synapse &Syn, float   &Delay )->void{Syn.DelayinTsteps = Delay*IArgs.onemsbyTstep + 0.5f; }));

		Neurons.resize(N);
		MexTransform(IArgs.a.begin(), IArgs.a.end(), Neurons.begin(), FFL([](Neuron &Neu, float &a)->void{Neu.a = a; }));
		MexTransform(IArgs.b.begin(), IArgs.b.end(), Neurons.begin(), FFL([](Neuron &Neu, float &b)->void{Neu.b = b; }));
		MexTransform(IArgs.c.begin(), IArgs.c.end(), Neurons.begin(), FFL([](Neuron &Neu, float &c)->void{Neu.c = c; }));
		MexTransform(IArgs.d.begin(), IArgs.d.end(), Neurons.begin(), FFL([](Neuron &Neu, float &d)->void{Neu.d = d; }));

		// Setting Initial Conditions of V and U
		if (U.istrulyempty()){
			U.resize(N);
			for (int j = 0; j<N; ++j)
				U[j] = Neurons[j].b*(Neurons[j].b - 5.0f - sqrt((5.0f - Neurons[j].b)*(5.0f - Neurons[j].b) - 22.4f)) / 0.08f;
		}
		else if (U.size() != N){
			// GIVE ERROR MESSAGE HERE
			return;
		}

		if (V.istrulyempty()){
			V.resize(N);
			for (int j = 0; j<N; ++j){
				V[j] = (Neurons[j].b - 5.0f - sqrt((5.0f - Neurons[j].b)*(5.0f - Neurons[j].b) - 22.4f)) / 0.08f;
			}
		}
		else if (V.size() != N){
			// GIVE ERROR MESSAGE HEREx
			return;
		}

		// Setting Initial Conditions for INTERNAL CURRENT 1
		if (IArgs.InitialState.Iin1.size() == N){
			for (int j = 0; j < N; ++j){
				Iin1[j] = (int64_t)(IArgs.InitialState.Iin1[j] * (1LL << 32));
			}
		}
		else if (IArgs.InitialState.Iin1.size()){
			// GIVE ERROR MESSAGE HERE
			return;
		}
		//else{
		//	Iin1 is already initialized to zero by tbb::zero_allocator<int64_t>
		//}

		// Setting Initial Conditions for INTERNAL CURRENT 2
		if (IArgs.InitialState.Iin2.size() == N){
			for (int j = 0; j < N; ++j){
				Iin2[j] = (int64_t)(IArgs.InitialState.Iin2[j] * (1LL << 32));
			}
		}
		else if (IArgs.InitialState.Iin2.size()){
			// GIVE ERROR MESSAGE HERE
			return;
		}
		//else{
		//	Iin2 is already initialized to zero by tbb::zero_allocator<int64_t>
		//}

		// Setting Initial Conditions for Weight Derivative
		if (WeightDeriv.istrulyempty()){
			WeightDeriv.resize(M, 0.0f);
		}
		else if (WeightDeriv.size() != M){
			// Return Exception
			return;
		}

		// Initializing InternalVars for IextInternal giving it
		//   1. The IExtInterface::InternalVarsStruct in InternalVars
		//   2. The IExtInterface::InputVarsStruct in InputVars
		//   3. The IExtInterface::SingleStateStruct (Initial State) in InputVars
		//   4. InputVars itself to take parameters such as N, onemsbyTstep, 
		//      Noofms etc.
		
		IExtInterface::initInternalVariables(
			IextInterface,
			IArgs.IextInterface,
			IArgs.InitialState.IextInterface,
			IArgs
		);
		
		// Setting Initial Conditions of SpikeQueue
		if (IArgs.InitialState.SpikeQueue.istrulyempty()){
			SpikeQueue = MexVector<MexVector<int32_t> >(onemsbyTstep * DelayRange, MexVector<int32_t>());
		}
		else if (IArgs.InitialState.SpikeQueue.depth() == 1 && IArgs.InitialState.SpikeQueue.LevelSize(0) == onemsbyTstep*DelayRange) {
			IArgs.InitialState.SpikeQueue.getVectTree(SpikeQueue);
		}
		else {
			//GIVE ERROR MESSAGE HERE
			return;
		}

		// Setting Initial Conditions for LSTs
		if (LSTNeuron.istrulyempty()){
			LSTNeuron = MexVector<int32_t>(N, -1);
		}
		else if (LSTNeuron.size() != N){
			//GIVE ERROR MESSAGE HERE
			return;
		}
		if (LSTSyn.istrulyempty()){
			LSTSyn = MexVector<int32_t>(M, -1);
		}
		else if (LSTSyn.size() != M){
			//GIVE ERROR MESSAGE HERE
			return;
		}
	}
	void DoOutput(StateVarsOutStruct &StateOut, OutputVarsStruct &OutVars);
	void DoSingleStateOutput(SingleStateStruct &FinalStateOut);
	void DoInputStateOutput(InputArgs &InputStateOut);
};

struct OutputVarsStruct{
	MexMatrix<float> WeightOut;
	MexMatrix<float> Iin;
	MexMatrix<float> Itot;
	MexVector<uint64_t> NoOfSpikes;
	IExtInterface::OutputVarsStruct IextInterface;
	struct SpikeListStruct{
		MexVector<int32_t> SpikeSynInds;
		MexVector<int32_t> TimeRchdStartInds;
		MexVector<int32_t> TimeRchd;
		SpikeListStruct() : SpikeSynInds(), TimeRchdStartInds(), TimeRchd() {}
	} SpikeList;

	OutputVarsStruct() :
		WeightOut(),
		Itot(),
		Iin(),
		NoOfSpikes(),
		IextInterface(),
		SpikeList(){}

	void initialize(const InternalVars &);
};

struct StateVarsOutStruct{
	MexMatrix<float> WeightOut;
	MexMatrix<float> VOut;
	MexMatrix<float> UOut;
	MexMatrix<float> Iin1Out;
	MexMatrix<float> Iin2Out;
	MexMatrix<float> WeightDerivOut;
	IExtInterface::StateOutStruct IextInterface;

	MexVector<int32_t> TimeOut;
	FlatVectTree<int32_t> SpikeQueueOut;
	MexVector<int32_t> CurrentQIndexOut;
	MexMatrix<int32_t> LSTNeuronOut;
	MexMatrix<int32_t> LSTSynOut;

	StateVarsOutStruct() :
		WeightOut(),
		VOut(),
		UOut(),
		Iin1Out(),
		Iin2Out(),
		WeightDerivOut(),
		IextInterface(),
		TimeOut(),
		SpikeQueueOut(),
		CurrentQIndexOut(),
		LSTNeuronOut(),
		LSTSynOut() {}

	void initialize(const InternalVars &);
};


void CountingSort(int N, MexVector<Synapse> &Network, MexVector<int32_t> &indirection);

void SimulateParallel(
	InputArgs &&InputArguments,
	OutputVarsStruct &PureOutputs,
	StateVarsOutStruct &StateVarsOutput,
	SingleStateStruct &FinalStateOutput,
	InputArgs &InitalStateOutput);

#endif
