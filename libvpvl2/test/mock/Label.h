#include <vpvl2/ILabel.h>

namespace vpvl2 {

class MockILabel : public ILabel {
 public:
  MOCK_CONST_METHOD0(name,
      const IString*());
  MOCK_CONST_METHOD0(englishName,
      const IString*());
  MOCK_CONST_METHOD0(isSpecial,
      bool());
  MOCK_CONST_METHOD0(count,
      int());
  MOCK_CONST_METHOD1(bone,
      IBone*(int index));
  MOCK_CONST_METHOD1(morph,
      IMorph*(int index));
};

}  // namespace vpvl2
