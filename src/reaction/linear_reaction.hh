/** @brief class Linear_reaction is used to enable simulation of simple chemical reactions
 *
 * Class in this file makes it possible to realize  simulation of reaction of the first order by simple matrix multiplication.
 * One step of the linear reaction is represented as a product of a matrix containing concentrations of observed speciesin elements in rows multiplied by so called
 * reaction_matrix. Through this way radioactive decay can bee also realized and that was exactly what we did at the begining of journey. :-)
 * Matrix containing concentrations has a dimension Nxn, where N is a number of elements in mesh and n denotes a number of transported chemical species.
 * The reaction_matrix is a square matrix and it has a dimension nxn.
 *
 */
#ifndef LINREACT
#define LINREACT

#include <vector>
#include "reaction/reaction.hh"

class Linear_reaction: public Reaction
{
	public:
        /**
         *  Constructor with parameter for initialization of a new declared class member
         *  TODO: parameter description
         */
		Linear_reaction(TimeMarks &marks, Mesh &init_mesh, MaterialDatabase &material_database); //(double timeStep, Mesh * mesh, int nrOfSpecies, bool dualPorosity); //(double time_step, int nrOfElements, double ***ConcentrationMatrix);
		/**
		*	Destructor.
		*/
		~Linear_reaction(void);
		/**
		*	For simulation of chemical raection in just one element either inside of MOBILE or IMMOBILE pores.
		*/
		double **compute_reaction(double **concentrations, int loc_el);
		/**
		*	Prepared to compute simple chemical reactions inside all of considered elements. It calls compute_reaction(...) for all the elements controled by concrete processor, when the computation is paralelized.
		*/
		void compute_one_step(void);
		/**
		*	This method enables to change the timestep for computation of simple chemical reactions. Such a change is conected together with creating of a new reaction matrix necessity.
		*/
		void set_time_step(double new_timestep);
		/**
		* Folowing method enabels the timestep for chemistry to have the value written in ini-file.
		*/
		void set_time_step(void);
	protected:
		/**
		*	This method reads a sequence of numbers defining an order of substances in decay chain. The string section defines where too look for indices inside of ini-file, whereas n_subst is a number of isotopes in described decay chain.
		*/
		int *set_indeces(char *section, int n_subst);
		/**
		*	This method reads an information about a number of isotopes in a decay chain described inside of ini-file in section given as an argument. This method is used for radioactive decay simulation.
		*/
		void set_nr_of_isotopes(char* section);
		/**
		*	This method sets number of isotopes for the case of first order reaction. The value should be always 2.
		*/
		void set_nr_of_isotopes(int Nr_of_isotopes);
		/**
		*	This method reads a sequence of (nr_of_isotopes - 1) halflives belonging to separate decay chain step. This information is placed in ini-file in a block starting with a string section.
		*/
		double *set_half_lives(char *section);
		/**
		*	This method reads form ini-file an information for construction of a matrix describing bifurcation of every single decay chain on one row of the reaction matrix. Informations about bifurcation are placed in a block starting with a string section. dec_nr identifies which one decay chain is handled and which row of twodimensional bifurcation matrix (double **array)should be affected.
		*/
		void set_bifurcation(char *section, int dec_nr);
		/**
		*	This method reads from ini-file an information if the bifurcation for a current decay chain is switched on in a block starting with a string section. Initialy bifurcation is switched of.
		*/
		void set_bifurcation_on(char *section);
		/**
		*	This method reads from ini-file an information if first order reactions simulation is switched on.
		*/
		void set_For_on(void);
		/**
		*	This method reads from ini-file an information if a radioactive decay simulation is switched on.
		*/
		void set_decay_on(void);
		/**
		*	This method reads from ini-file an information and prepares a vector (onedimensional double *array) containing kinetic constants of every single first order reactions. Those informations are placed in a block with a string section at the beginning. From those constants half-lives belonging to first order reactions are computed.
		*/
		void set_kinetic_constants(char *section, int reaction_nr);
		/**
		*
		*/
		double **allocate_reaction_matrix(void);
		/**
		*	This method modificates reaction matrix as described in ini-file a single section [Decay_i] or [FoReact_i]. It is used when bifurcation is switched off.
		*/
		double **modify_reaction_matrix(void);
		/**
		*	This method modificates reaction matrix as described in ini-file a single section [Decay_i] or [FoReact_i]. It is used when bifurcation is switched on.
		*/
		double **modify_reaction_matrix(int bifurcation); ///< it is used for reaction matrix modification in cases when a bifurcation for a current decay chain is switched on
		/**
		*	This method calls modify_reaction_matrix(...) for every single decay and first order reaction.
		*/
		double **modify_reaction_matrix_repeatedly(void); ///< calls the function modify_reaction_matrix(..) so many times as many decays are defined
		/**
		*	For control printing of a matrix describing simple chemical raections.
		*/
		void print_reaction_matrix(void);
		/**
		*	For printing nr_of_isotopes identifies of isotopes in a current decay chain.
		*/
		void print_indeces(int n_subst);
		/**
		* Following method releases reaction matrix to make it possible to set a new time step for chemistry.
		*/
		void release_reaction_matrix();
		/**
		*	For printing (nr_of_isotopes - 1) doubles containing half-lives belonging to particular isotopes on screen.
		*/
		void print_half_lives(int n_subst);
		/**
		*	Small (nr_of_species x nr_of_species) square matrix for realization of radioactive decay and first order reactions simulation.
		*/
		double **reaction_matrix;
		//std::vector<double> half_lives; ///< alternative to following row
		/**
		*	Sequence of (nr_of_isotopes - 1) doubles containing half-lives belonging to particular isotopes.
		*/
		double *half_lives;
		/**
		*	Sequence of integers describing an order of isotopes in decay chain or first order reaction.
		*/
		int *substance_ids;
		/**
		*	Informs about the number of isotopes in a current decay chain.
		*/
		int nr_of_isotopes;
		/**
		*	Two dimensional array contains mass percentage of every single decay bifurcation on every single row.
		*/
		std::vector<std::vector<double> > bifurcation;
		/**
		*	One dimensional array of kinetic constants belonging to considered reactions.
		*/
		std::vector<double> kinetic_constant;
		/**
		*	Boolean which enables to turn on branching of considered decay chain.
		*/
		bool bifurcation_on;
};

#endif
