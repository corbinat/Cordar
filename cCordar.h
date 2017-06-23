// This class is used to load versatile configuration files called Cordar.
// CORbin's DAta Representation

#ifndef ___cCordar_h___
#define ___cCordar_h___

///////////////////////////////////////////////////////////////////////
//
// Includes
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include <cstdint>


///////////////////////////////////////////////////////////////////////
//
// Class Declaration
//
//////////////////////////////////////////////////////////////////////

class cCordar
{
public:
   cCordar();
   ~cCordar();

   cCordar& operator[](std::string a_Key);
   cCordar& operator[](int a_Index);

   cCordar& operator= (const std::string & a_rValue);
   bool operator==(const std::string & a_rValue);
   operator std::string() const;

   // Similar to operator[] except it pulls out the whole node, not the value.
   // Don't need an index version becaues it would be the same as [] in that
   // case (Think about it!).
   cCordar Copy(std::string a_Key);

   bool Merge(cCordar a_Other, bool a_OverwriteOnConflict = false);

   // Implementation detail: Pulls from m_Child map instead of vector
   std::pair<std::string, cCordar> GetPropertyByIndex(size_t a_Index);

   bool LoadFile(std::string a_File);
   void LoadStream(std::istream& a_rStream, cCordar * a_pRoot = NULL);

   bool SerializeToFile(std::string a_FileName);
   void SerializeToStream(std::ostream& a_rStream);

   std::string GetProperty(std::string a_Property);
   void SetProperty(std::string a_Property, std::string a_Value);

   // This function returns the size of only this level, not children levels.
   size_t Size() const;

   // If this is a "m_Child" node, delete element with this name.
   // If this is a "m_Vector" node, assume index 0.
   bool Delete(std::string a_Key);

   // If this is a "m_Vector" node, delete element at this value
   bool Delete(size_t a_Index);

private:

   void _SerializeToStream(
      std::ostream& a_rStream,
      cCordar * a_pRoot = NULL,
      uint32_t a_TabLevel = 0,
      bool a_NeedsTab = true
      );

   // Used when exporting to stream/file
   std::string _TabLevelToString(uint32_t a_TabLevel);

   // Helper function that returns true if this node contains a vector of
   // other nodes. We can't just check m_Vector.size() because there is a
   // special case where the vector may contain one empty node. We want this
   // special case to be considered empty as well.
   bool _ContainsVectorofNodes(cCordar * a_pRoot = NULL) const;

   // This map will contain nodes that only have a value or nodes that have
   // vectors of other nodes.
   std::map<std::string, cCordar> m_Child;

   // All of the nodes in the vector will have a "child" map
   std::vector<cCordar> m_Vector;

   std::string m_Value;
};

#endif
