#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <std_compat/optional.h>

class Trie
{
public:
  Trie()
    : root(std::make_shared<TrieNode>(std::weak_ptr<TrieNode>()))
  {}
  template <class ForwardIt>
  Trie(ForwardIt begin_it, ForwardIt end_it)
    : Trie()
  {
    std::for_each(begin_it, end_it,
                  [this](auto const& candidate) { this->insert(candidate); });
  }

  void insert(std::string const& to_insert)
  {
    size_t last_id = Trie::ambiguous;
    std::shared_ptr<TrieNode> current = root;
    for (char character : to_insert) {
      if (!current->has_any_child() && last_id==Trie::ambiguous && current != root) {
        throw std::logic_error("two more more keys are completely ambiguous");
      } else { 
        if(current->get_id() != max_id) current->set_id(Trie::ambiguous);
        if (current->has_child(character)) {
          current = current->get_child(character);
        } else {
          current = current->create_child(character, max_id);
          last_id = max_id;
        }
      }
    }
    if(last_id == Trie::ambiguous) throw std::logic_error("two more more keys are completely ambiguous");
    max_id++;
  }

  compat::optional<size_t> find(std::string const& to_match) { 
    std::shared_ptr<TrieNode> current = root;

    for (auto character : to_match) {
      if(!current->has_child(character)) return std::nullopt;
      current = current->get_child(character);
      if(current->get_id() != ambiguous) return current->get_id();
    }
    return std::nullopt;
  }

private:
  class TrieNode : public std::enable_shared_from_this<TrieNode>
  {
  public:
    TrieNode(std::weak_ptr<TrieNode> parent, char character = '\0',
             size_t id = -1)
      : parent(parent)
      , character(character)
      , id(id)
    {}
    bool has_child(char character) const
    {
      return children.find(character) != children.end();
    }
    bool has_any_child() const {
      return not children.empty();
    }
    std::shared_ptr<TrieNode> get_child(char character) const
    {
      return children.find(character)->second;
    }
    std::shared_ptr<TrieNode> create_child(char character, size_t id = -1)
    {
      return children[character] =
               std::make_shared<TrieNode>(weak_from_this(), character, id);
    }
    size_t get_id() {
      return id;
    }


  private:
    void set_id(size_t id) {
      this->id = id;
    }

    std::weak_ptr<TrieNode> parent;
    std::map<char, std::shared_ptr<TrieNode>> children;
    char character;
    size_t id;
    friend Trie;
  };

  static const size_t ambiguous = -1;
  size_t max_id = 0;
  std::shared_ptr<TrieNode> root;
};

/**
 * fuzzy matches text against the longest unique given set of candidates
 * specified by the input iterator range
 */
template <class ForwardIterator>
compat::optional<size_t>
fuzzy_match(std::string const& text, ForwardIterator begin_it, ForwardIterator end_it)
{
  Trie candidates(begin_it, end_it);
  return candidates.find(text);
}
