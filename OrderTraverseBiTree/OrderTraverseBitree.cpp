/*
struct TreeNode {
    char data;
    struct TreeNode *left;
    struct TreeNode *right;
    TreeNode(int x) :
            val(x), left(NULL), right(NULL) {
    }
};*/
 
 
 
 
class Solution {
public:
    void PreOrder(TreeNode *pRoot)
    {
        cout<<pRoot->data<<endl;
        PreOrder(pRoot->left);
        PreOrder(pRoot->right);
    }//前序遍历
  
  
   void InOrder(TreeNode *pRoot)
    {
        InOrder(pRoot->left);
        cout<<pRoot->data<<endl;
        InOrder(pRoot->right);
    }//中序遍历
  
  
   void PostOrder(TreeNode *pRoot)
    {
        PostOrder(pRoot->left);
        PostOrder(pRoot->right);
        cout<<pRoot->data<<endl;
    }//后序遍历
};
