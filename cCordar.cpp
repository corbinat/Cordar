///////////////////////////////////////////////////////////////////////
//
// Includes
//
//////////////////////////////////////////////////////////////////////
#include "cCordar.h"

#include <iostream>
#include <fstream>

///////////////////////////////////////////////////////////////////////
//
// Static Helpers
//
//////////////////////////////////////////////////////////////////////

// Try to find strings that are unlikely to appear in the data. May need to
// find a better solution to this in the future
const std::string g_kColon = ";!;!;";
const std::string g_kOpenCurly = ";![!;";
const std::string g_kCloseCurly = ";!]!;";

static std::vector<std::string> ExplodeString(
   std::string a_String,
   char a_Delim,
   bool & a_rDelimFound
   )
{
   a_rDelimFound = false;

   std::vector<std::string> l_Result;
   std::string l_Part;
   for (std::string::iterator i = a_String.begin(); i != a_String.end(); ++i)
   {
      if (*i != a_Delim)
      {
         l_Part += *i;
      }
      else
      {
         a_rDelimFound = true;

         // Remove leading and trailing whitespace
         size_t l_Start = l_Part.find_first_not_of(" \t\r\n");
         size_t l_End = l_Part.find_last_not_of(" \t\r\n");

         if (l_Start != std::string::npos && l_End != std::string::npos)
         {
            l_Part = l_Part.substr(l_Start, l_End + 1 - l_Start);
            l_Result.push_back(l_Part);
         }

         l_Part.clear();

      }
   }

   if (!l_Part.empty())
   {
      // Remove leading and trailing whitespace
      size_t l_Start = l_Part.find_first_not_of(" \t\r\n");
      size_t l_End = l_Part.find_last_not_of(" \t\r\n");

      if (l_Start != std::string::npos && l_End != std::string::npos)
      {
         l_Part = l_Part.substr(l_Start, l_End + 1 - l_Start);
         l_Result.push_back(l_Part);
      }
   }

   return l_Result;
}

// There are a few special characters for formatting that can't be within the
// data. These include the following    : { }
static std::string ReplaceAll(
   std::string & a_Input,
   std::string a_From,
   std::string a_To
   )
{
   std::string l_NewString;
   l_NewString.reserve(a_Input.length());  // avoids a few memory allocations

   std::string::size_type l_LastPos = 0;
   std::string::size_type l_FindPos;

   while((l_FindPos = a_Input.find(a_From, l_LastPos)) != std::string::npos)
   {
       l_NewString.append(a_Input, l_LastPos, l_FindPos - l_LastPos);
       l_NewString += a_To;
       l_LastPos = l_FindPos + a_From.length();
   }

   // Care for the rest after last occurrence
   l_NewString += a_Input.substr(l_LastPos);

   return l_NewString;
}

static std::string Sanitize(const std::string & a_Input)
{
   if (a_Input.empty())
   {
      return a_Input;
   }

   std::string l_NewString = a_Input;
   l_NewString = ReplaceAll(l_NewString, ":", g_kColon);
   l_NewString = ReplaceAll(l_NewString, "{", g_kOpenCurly);
   l_NewString = ReplaceAll(l_NewString, "}", g_kCloseCurly);

   return l_NewString;
}

static std::string Unsanitize(std::string & a_Input)
{
   if (a_Input.empty())
   {
      return a_Input;
   }

   std::string l_NewString = a_Input;
   l_NewString = ReplaceAll(l_NewString, g_kColon, ":");
   l_NewString = ReplaceAll(l_NewString, g_kOpenCurly, "{");
   l_NewString = ReplaceAll(l_NewString, g_kCloseCurly, "}");

   return l_NewString;
}

static bool IsComment(const std::string & a_rkLine)
{
   size_t l_FirstChar = a_rkLine.find_first_not_of(" \t\r\n");
   if (l_FirstChar != std::string::npos)
   {
      if (a_rkLine[l_FirstChar] == '#')
      {
         // This line is a comment.
         return true;
      }
   }

   return false;
}

///////////////////////////////////////////////////////////////////////
//
// Class Definition
//
//////////////////////////////////////////////////////////////////////
cCordar::cCordar()
   :m_Child(),
    m_Vector(),
    m_Value()
{
}

cCordar::~cCordar()
{
}

cCordar& cCordar::operator[](std::string a_Key)
{
   cCordar * l_pItem;

   // If this is an array but the user is trying to access a property in the
   // array, assume index 0.
   if (m_Vector.size() > 0)
   {
      l_pItem = &(m_Vector.begin()->m_Child[a_Key]);
   }
   else
   {
      l_pItem = &(m_Child[a_Key]);
   }

   if (l_pItem->m_Vector.size() == 0)
   {
      // This category didn't exist before, but we implicitly created it through
      // the map access. Need to create the first element in the category's
      // array.
      cCordar l_EmptyNode;
      l_pItem->m_Vector.push_back(l_EmptyNode);
   }

   return *(l_pItem);
}

cCordar& cCordar::operator[](int a_Index)
{
   // If this is not a vector (it already has children in the map) but the user
   // is trying to access an index we can do one of 3 things:
   // 1. Translate this into a similar call as GetPropertyByIndex
   // 2. Ignore it and return "this"
   // 3. Thow away all of the children and convert this to a vector

   // I'm going to go with #3 because it feels the most intuitive?

   m_Child.clear();

   if (static_cast<size_t>(a_Index + 1) > m_Vector.size())
   {
      m_Vector.resize(a_Index + 1);
   }
   return m_Vector[a_Index];
}

cCordar& cCordar::operator= (const std::string & a_rValue)
{
   m_Value = a_rValue;
   return *this;
}

bool cCordar::operator==(const std::string & a_rValue)
{
   return m_Value == a_rValue;
}

cCordar::operator std::string() const
{
   return m_Value;
}

cCordar cCordar::Copy(std::string a_Key)
{
   cCordar l_Copy;
   l_Copy[a_Key] = operator[](a_Key);
   return l_Copy;
}

bool cCordar::Merge(cCordar a_Other, bool a_OverwriteOnConflict)
{
   for (auto i = a_Other.m_Child.begin(); i != a_Other.m_Child.end(); ++i)
   {
      // Merging in other map of child nodes. Our node better not have a vector
      // because then screwy things can happen when serializing to stream.
      if (_ContainsVectorofNodes())
      {
         if (a_OverwriteOnConflict)
         {
            m_Vector.clear();
         }
         else
         {
            return false;
         }
      }

      std::map<std::string, cCordar>::iterator it = m_Child.find(i->first);
      if (it != m_Child.end())
      {
         // This child already exists so go deeper in case this is a category
         operator[](i->first).Merge(a_Other[i->first], a_OverwriteOnConflict);
      }
      else
      {
         m_Child[i->first] = i->second;
      }
   }

   for (size_t i = 0; i < a_Other.m_Vector.size(); ++i)
   {
      // Merging in other vector of nodes. Our node better not have a child map
      // because then screwy things can happen when serializing to stream.
      if (m_Child.size() != 0)
      {
         if (a_OverwriteOnConflict)
         {
            m_Child.clear();
         }
         else
         {
            return false;
         }
      }

      if (m_Vector.size() > i)
      {
         m_Vector[i].Merge(a_Other.m_Vector[i], a_OverwriteOnConflict);
      }
      else
      {
         m_Vector.push_back(a_Other.m_Vector[i]);
      }
   }

   if (a_OverwriteOnConflict && !a_Other.m_Value.empty())
   {
      m_Value = a_Other.m_Value;
   }

   return true;
}

std::pair<std::string, cCordar> cCordar::GetPropertyByIndex(size_t a_Index)
{
   cCordar * l_pItem = this;

   // If this is an array but the user is trying to access a property in the
   // array, assume index 0.
   if (m_Vector.size() > 0)
   {
      l_pItem = &(m_Vector.front());
   }

   std::pair<std::string, cCordar> l_Return;
   if (a_Index > l_pItem->m_Child.size())
   {
      return l_Return;
   }

   auto l_Iter = l_pItem->m_Child.begin();
   for (size_t i = 0; i < a_Index; ++i)
   {
      ++l_Iter;
   }

   return *l_Iter;
}

bool cCordar::LoadFile(std::string a_FileName)
{
   std::ifstream l_FileStream;
   l_FileStream.open(a_FileName.c_str());
   if (l_FileStream.fail())
   {
      l_FileStream.close();
      return false;
   }

   LoadStream(l_FileStream);
   l_FileStream.close();

   return true;
}

void cCordar::LoadStream(std::istream& a_rStream, cCordar * a_pRoot)
{
   if (a_pRoot == NULL)
   {
      a_pRoot = this;
   }

   cCordar * l_pLastCreatedNode = a_pRoot;
   int32_t l_ArrayDepth = 0;

   // start reading the file line by line
   std::string l_Line;
   while (getline(a_rStream, l_Line))
   {
      if (IsComment(l_Line))
      {
         continue;
      }

      std::vector<std::string> l_Explode;
      bool l_DelimFound = false;
      l_Explode = ExplodeString(l_Line, ':', l_DelimFound);

      if (l_Explode.size() == 0)
      {
         // This is a blank line
         continue;
      }
      else if (l_Explode.size() == 2)
      {
         // If there is something on the other side of the colon then this is
         // just a simple property.
         (*a_pRoot)[Unsanitize(l_Explode[0])] = Unsanitize(l_Explode[1]);
         l_pLastCreatedNode = &(*a_pRoot)[l_Explode[0]];

         // Not in an array so reset the count
         l_ArrayDepth = 0;
      }
      else if (l_Explode.size() == 1)
      {
         if (l_Explode[0] == "{")
         {
            LoadStream(a_rStream, &(*l_pLastCreatedNode)[l_ArrayDepth]);
            ++l_ArrayDepth;
         }
         else if (l_Explode[0] == "}")
         {
            return;
         }
         else
         {
            if (!l_DelimFound)
            {
               // There was no delimeter, so this is just a raw value
               (*a_pRoot) = Unsanitize(l_Explode[0]);
            }
            else
            {
               // This will implicitly create a new category or empty property
               l_pLastCreatedNode = &(*a_pRoot)[Unsanitize(l_Explode[0])];

               // Not in an array so reset the count
               l_ArrayDepth = 0;
            }
         }
      }
   }
}

bool cCordar::SerializeToFile(std::string a_FileName)
{
   std::ofstream l_File;
   l_File.open(a_FileName.c_str());
   if (l_File.fail())
   {
      l_File.close();
      return false;
   }

   SerializeToStream(l_File);
   l_File.close();

   return true;
}

void cCordar::SerializeToStream(std::ostream& a_rStream)
{
   _SerializeToStream(a_rStream);
}

std::string cCordar::GetProperty(std::string a_Property)
{
   if (m_Vector.size() > 0 && m_Vector.begin()->Size() != 0)
   {
      // This node contains a vector and the vector contains more than just a
      // placeholder node. Assume index 0.
      return m_Vector.begin()->m_Child[a_Property].m_Value;
   }
   else
   {
      return m_Child[a_Property].m_Value;
   }
}

void cCordar::SetProperty(std::string a_Property, std::string a_Value)
{
   if (m_Vector.size() > 0 && m_Vector.begin()->Size() != 0)
   {
      // This node contains a vector. Assume index 0.
      m_Vector.begin()->m_Child[a_Property].m_Value = a_Value;
   }
   else
   {
      m_Child[a_Property].m_Value = a_Value;
   }
}

size_t cCordar::Size() const
{
   if (m_Child.size() != 0)
   {
      return m_Child.size();
   }
   else if (_ContainsVectorofNodes())
   {
      return m_Vector.size();
   }
   else if (m_Value.size() != 0)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

bool cCordar::Delete(std::string a_Key)
{
   cCordar * l_pItem = this;

   // If this is an array but the user is trying to access a property in the
   // array, assume index 0.
   if (m_Vector.size() > 0)
   {
      l_pItem = &(m_Vector.front());
   }

   size_t l_ItemsErased = l_pItem->m_Child.erase(a_Key);
   return l_ItemsErased != 0;
}

bool cCordar::Delete(size_t a_Index)
{
   if (a_Index < m_Vector.size())
   {
      std::vector<cCordar>::iterator i = m_Vector.begin() + a_Index;
      m_Vector.erase(i);
      return true;
   }
   return false;
}

void cCordar::_SerializeToStream(
   std::ostream& a_rStream,
   cCordar * a_pRoot,
   uint32_t a_TabLevel,
   bool a_NeedsTab
   )
{
   if (a_pRoot == NULL)
   {
      a_pRoot = this;
   }

   if (static_cast<std::string>(*a_pRoot).empty() == false)
   {
      // Indent only if the last thing in the stream is a new line. Otherwise
      // stuff is before us and we don't need to indent.
      if (a_NeedsTab)
      {
         a_rStream << _TabLevelToString(a_TabLevel);
      }
      std::string l_Value = Sanitize(static_cast<std::string>(*a_pRoot));
      a_rStream << l_Value << '\n';
   }

   for (std::map<std::string, cCordar>::iterator i = a_pRoot->m_Child.begin();
      i != a_pRoot->m_Child.end();
      ++i
      )
   {
      std::string l_Key = Sanitize(i->first);
      a_rStream << _TabLevelToString(a_TabLevel) << l_Key << ": ";
      if (static_cast<std::string>(i->second).empty())
      {
         // The child is starting a vector so we shouldn't expect anything after
         // the colon. Print a new line ourselves.
         a_rStream << '\n';
      }

      _SerializeToStream(a_rStream, &(i->second), a_TabLevel, false);
   }

   if (_ContainsVectorofNodes(a_pRoot))
   {
      for (std::vector<cCordar>::iterator i = a_pRoot->m_Vector.begin();
         i != a_pRoot->m_Vector.end();
         ++i
         )
      {
         //if (i->Size() != 0)
         {
            a_rStream << _TabLevelToString(a_TabLevel) << "{\n";
            _SerializeToStream(a_rStream, &(*i), a_TabLevel + 1);
            a_rStream << _TabLevelToString(a_TabLevel) << "}\n";
         }
      }
   }
}

std::string cCordar::_TabLevelToString(uint32_t a_TabLevel)
{
   std::string l_ReturnString;
   for (uint32_t i = 0; i < a_TabLevel; ++i)
   {
      l_ReturnString += "   ";
   }
   return l_ReturnString;
}

bool cCordar::_ContainsVectorofNodes(cCordar * a_pRoot) const
{
   const cCordar * l_Root = a_pRoot ? a_pRoot : this;

   // Return true if vector has 1 (or more) non-empy node.
   return l_Root->m_Vector.size() != 0
          && !(l_Root->m_Vector.size() == 1 && l_Root->m_Vector.begin()->Size() == 0);
}
