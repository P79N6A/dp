/**
**/
#ifndef POSEIDON_BUDGET_PDB_PACING_H
#define POSEIDON_BUDGET_PDB_PACING_H

#include <vector>
#include <time.h>


#include "types.h"


#include "campaign_budget.h"
#include "poseidon_ors_offline.pb.h"
#include "poseidon_ors_model.pb.h"


using namespace std;
using namespace poseidon::ors_offline;


/**
 *
 * @param file_path
 * @param campaign_budgets
 * @param t
 * @param compaign_type
 * @param campaign_adcount
 * @return
 */
int getPDBDealBudgets(const string &file_path,
                          vector<poseidon::ors_offline::pdbCampaignBudget> &campaign_budgets,
                          const time_t &t,
                          int compaign_type,
                          map<string, set<UInt32> > &mp_deal2campaign);


/**
 *
 * @param config : config class for config files
 * @param campaign_bidding_rate: current bidding_rate
 * @param campaign_posted_budget: pasted exp for each campaign
 * @param history_posted_budget_smooth: history
 * @param campaign_budgets: each init campaign class
 * @param t
 * @param budget_pacing_model
 * @param campaign_adcount: campaign has the number of ads
 * @return
 */
int updatePDBBudgetPacingModel(const poseidon::ors_offline::BudgetPacingConfig &config,
                               map<UInt32, float> &campaign_bidding_rate,
                               map<UInt32, int> &campaign_posted_budget,
                               map<UInt32, float> &history_posted_budget_smooth,
                               vector<poseidon::ors_offline::pdbCampaignBudget> &campaign_budgets,
                               time_t t,
                               poseidon::ors::BudgetPacingModel &budget_pacing_model,
                               map<string, set<UInt32> > &mp_deal2campaign);

#endif //POSEIDON_BUDGET_PDB_PACING_H
