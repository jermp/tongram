#include <iostream>
#include <limits>
#include <cmath>

#include "utils/util.hpp"
#include "utils/iterators.hpp"
#include "lm_types.hpp"
#include "score.hpp"
#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"

using namespace tongrams;

template <typename Model>
void score_corpus(std::string const& index_filename,
                  std::string const& corpus_filename) {
    Model model;
    essentials::logger("Loading data structure");
    util::load(model, index_filename);
    text_lines corpus(corpus_filename.c_str());

    uint64_t tot_OOVs = 0;
    uint64_t corpus_sentences = 0;
    float tot_log10_prob = 0.0;
    float tot_log10_prob_only_OOVs = 0.0;

    auto state = model.state();
    essentials::logger("Scoring");

    essentials::timer_type timer;
    timer.start();
    while (!corpus.end_of_file()) {  // assume one sentence per line
        state.init();
        float sentence_log10_prob = 0.0;
        bool is_OOV = false;

        // std::cerr << "{";

        corpus.begin_line();
        while (!corpus.end_of_line()) {
            auto word = corpus.next_word();
            float log10_prob = model.score(state, word, is_OOV);

            // std::cerr << "\"word\" : \"" << std::string(word.first,
            // word.second) << "\", "
            //        << "\"log10_prob\" : " << log10_prob << "\n";

            sentence_log10_prob += log10_prob;
            if (is_OOV) {
                tot_log10_prob_only_OOVs += log10_prob;
                is_OOV = false;
            }
        }

        // std::cerr << "\"total\" : " << sentence_log10_prob << ", "
        //           << "\"OOVs\" : " << state.OOVs << "}" << std::endl;

        tot_OOVs += state.OOVs;
        tot_log10_prob += sentence_log10_prob;
        ++corpus_sentences;
    }
    timer.stop();
    uint64_t corpus_tokens = corpus.num_words();
    std::cout.precision(8);
    std::cout << "tot_log10_prob = " << tot_log10_prob << std::endl;
    std::cout << "tot_log10_prob_only_OOVs = " << tot_log10_prob_only_OOVs
              << std::endl;
    std::cout << "perplexity including OOVs = "
              << pow(10.0,
                     -(tot_log10_prob / static_cast<double>(corpus_tokens)))
              << std::endl;
    if (corpus_tokens - tot_OOVs) {
        std::cout << "perplexity excluding OOVs = "
                  << pow(10.0,
                         -((tot_log10_prob - tot_log10_prob_only_OOVs) /
                           (static_cast<double>(corpus_tokens - tot_OOVs))))
                  << std::endl;
    }
    std::cout << "OOVs = " << tot_OOVs << "\n"
              << "corpus tokens = " << corpus_tokens << "\n"
              << "corpus sentences = " << corpus_sentences << "\n"
              << "elapsed time: " << timer.elapsed() / 1000000 << " [sec]"
              << std::endl;
}

int main(int argc, char** argv) {
    cmd_line_parser::parser parser(argc, argv);
    parser.add("index_filename", "Index filename.");
    parser.add("corpus_filename", "Corpus filename.");
    if (!parser.parse()) return 1;

    auto index_filename = parser.get<std::string>("index_filename");
    auto corpus_filename = parser.get<std::string>("corpus_filename");
    auto model_string_type = util::get_model_type(index_filename);

    if (false) {
#define LOOP_BODY(R, DATA, T)                              \
    }                                                      \
    else if (model_string_type == BOOST_PP_STRINGIZE(T)) { \
        score_corpus<T>(index_filename, corpus_filename);

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, TONGRAMS_SCORE_TYPES);
#undef LOOP_BODY
    } else {
        std::cerr << "Error: score() not supported with type "
                  << "'" << model_string_type << "'." << std::endl;
    }

    return 0;
}
