/*
struct TreeNode {
	int val;
	struct TreeNode *left;
	struct TreeNode *right;
	TreeNode(int x) :
			val(x), left(NULL), right(NULL) {
	}
};*/


//自己写的，有很多不足
class Solution {
public:
    vector<int> PrintFromTopToBottom(TreeNode* root)
    {
        TreeNode *queue[1000];    //不足1：本来想写TreeNode *queue = new TreeNode，不知道为什么一直报错，就直接改成1000了
        vector<int>result;
        int f=0;
        int r=0;                  //不足2：用了很多变量，很绕。r为存入队列的个数，r为当前队列已经输出个数，p指向第r个，为树节点的地址
        if(root!=NULL)
            queue[++r]=root;      //不足3：用的伪队列......
        while(f!=r)
        {
            TreeNode *p=queue[++f];
            result.push_back(p->val);
            if(p->left!=NULL)
                queue[++r]=p->left;
            if(p->right!=NULL)
                queue[++r]=p->right;
        }
        return result;
    }
};


//看了他人的思路后，修改的（运用队列，每得出一个数据，就在队列push该节点的左右孩子，然后再pop该节点）
class Solution {
public:
    vector<int> PrintFromTopToBottom(TreeNode* root)
    {
        vector<int>result;;
        if(root==NULL)
            return result;
        queue<TreeNode*> q;
        q.push(root);
        while(!q.empty())
        {
            result.push_back(q.front()->val);
            if(q.front()->left!=NULL)
                q.push(q.front()->left);
            if(q.front()->right!=NULL)
                q.push(q.front()->right);
            q.pop();
        }
        return result;
    }
};
