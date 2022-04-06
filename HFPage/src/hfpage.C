#include <iostream>
#include <stdlib.h>
#include <memory.h>
#include <vector>
#include <algorithm>

#include "hfpage.h"
#include "buf.h"
#include "db.h"

// **********************************************************
// page class constructor

void HFPage::init(PageId pageNo)
{
	// fill in the body
	slotCnt = 0;
	usedPtr = MAX_SPACE - DPFIXED;
	freeSpace = MAX_SPACE - DPFIXED + sizeof(slot_t);
	prevPage = INVALID_PAGE;
	nextPage = INVALID_PAGE;
	curPage = pageNo;

	slot[0].length = EMPTY_SLOT;
	slot[0].offset = 0;
}

// **********************************************************
// dump page utlity
void HFPage::dumpPage()
{
	int i;

	cout << "dumpPage, this: " << this << endl;
	cout << "curPage= " << curPage << ", nextPage=" << nextPage << endl;
	cout << "usedPtr=" << usedPtr << ",  freeSpace=" << freeSpace

	<< ", slotCnt=" << slotCnt << endl;

	for (i = 0; i < slotCnt; i++)
	{
		cout << "slot[" << i << "].offset=" << slot[i].offset
		<< ", slot[" << i << "].length=" << slot[i].length << endl;
	}
}

// **********************************************************
PageId HFPage::getPrevPage()
{
	// fill in the body
	return prevPage;
}

// **********************************************************
void HFPage::setPrevPage(PageId pageNo)
{
	// fill in the body
	prevPage = pageNo;
}

// **********************************************************
PageId HFPage::getNextPage()
{
	// fill in the body
	return this->nextPage;
}

// **********************************************************
void HFPage::setNextPage(PageId pageNo)
{
	// fill in the body
	this->nextPage = pageNo;
}

// **********************************************************
// Add a new record to the page. Returns OK if everything went OK
// otherwise, returns DONE if sufficient space does not exist
// RID of the new record is returned via rid parameter.
Status HFPage::insertRecord(char *recPtr, int recLen, RID &rid)
{
	// fill in the body
	bool isEmptySlot = false;
	int i = 0;
	int slotNo = 0;

	if (available_space() >= recLen)
	{
		slotNo = slotCnt;
		while (i <= slotCnt && !isEmptySlot)
		{
			if (slot[i].offset == EMPTY_SLOT && slot[i].length >= recLen)
			{
				isEmptySlot = true;
				slotNo = i;
			}
			i++;
		}
		if (!isEmptySlot)
		{
			slotCnt++;
			freeSpace -= sizeof(slot_t);
		}

		slot[slotNo].offset = usedPtr - recLen;
		slot[slotNo].length = recLen;

		rid.slotNo = slotNo;
		rid.pageNo = curPage;

		freeSpace -= recLen;

		memmove(data + slot[slotNo].offset, recPtr, recLen);
		usedPtr -= recLen;

		return OK;
	}
	return DONE;
}

// **********************************************************
// Delete a record from a page. Returns OK if everything went okay.
// Compacts remaining records but leaves a hole in the slot array.
// Use memmove() rather than memcpy() as space may overlap.
Status HFPage::deleteRecord(const RID &rid)
{
	// fill in the body
	int delsno = rid.slotNo;
	if(this->empty()) {
		return FAIL;
	}

	if(slotCnt-1 == rid.slotNo)
	{
		--slotCnt;
	}
	slot_t delSlot = slot[delsno];
	int test= 0;
	
	if(slotCnt > 2 && delSlot.length != EMPTY_SLOT && delSlot.length != 0) 
	{
		slot_t prev = delSlot;

		vector<slot_t> v;
		vector<int> index;
		
		//compact records
		for(int i = 0; i < slotCnt; i++) {
			if(slot[i].length != EMPTY_SLOT && slot[i].offset < delSlot.offset && slot[i].length != 0){
				v.push_back(slot[i]);
				index.push_back(i);
			}
		}
		
		for(int i=0;i<v.size();i++){
			for(int j=i+1;j<v.size();j++){
				if(v[i].offset < v[j].offset){
					slot_t temp = v[i];
					v[i] = v[j];
					v[j] = temp;
					
					int tempInt = index[i];
					index[i] = index[j];
					index[j] = tempInt;
				}
			}
		}
		
		short prevOffset = delSlot.offset;
		slot_t cur;
		
		for(int i=0;i< v.size();i++){
			
			cur = v[i];
			char buff[1000];
			//printf("prevOffset: %d, curOffset: %d\n", prevOffset, cur.offset);
			//printf("prevOffset + (delSlot.length - cur.length): %d\n", prevOffset + (delSlot.length - cur.length));
			memmove(buff , data + cur.offset , cur.length);
			memmove(data + prevOffset + (delSlot.length - cur.length) , buff , cur.length);
			
			//printf("before offset: %d\n", slot[index[i]].offset);
			
			slot[index[i]].offset = prevOffset + (delSlot.length - cur.length);
			prevOffset = cur.offset;
			//printf("after offset: %d\n", slot[index[i]].offset);
			
			prev = slot[index[i]];
		}
		
	}
	
	usedPtr += delSlot.length;
	freeSpace += delSlot.length + sizeof(slot_t);
	slot[delsno].length = EMPTY_SLOT;
	slot[delsno].offset = EMPTY_SLOT;

	return OK;
}

// **********************************************************
// returns RID of first record on page
Status HFPage::firstRecord(RID &firstRid)
{
	if(this->empty()) {
		return DONE;
	}
	// fill in the body
	int i=0;
	while(i <= slotCnt) {
		if(this->slot[i].length != EMPTY_SLOT) {
			firstRid.pageNo = this->curPage;
			firstRid.slotNo = i;
			return OK;
		}
		i++;
	}
	return DONE;
	
}

// **********************************************************
// returns RID of next record on the page
// returns DONE if no more records exist on the page; otherwise OK
Status HFPage::nextRecord(RID curRid, RID &nextRid)
{
	// fill in the body
	if(this->empty()) {
		return FAIL;
	}

	int i = curRid.slotNo + 1;

	while(i < slotCnt) {
		if(slot[i].length != EMPTY_SLOT) {
			nextRid.pageNo = curPage;
			nextRid.slotNo = i;
			return OK;
		} 
		i++;   
	}
	return DONE;
}

// **********************************************************
// returns length and copies out record with RID rid
Status HFPage::getRecord(RID rid, char *recPtr, int &recLen)
{
	// fill in the body
	int sno = rid.slotNo;

	if(sno > slotCnt || sno < 0) {
		return DONE;
	}

	recLen = this->slot[sno].length;

	if(recLen == EMPTY_SLOT) {
		return FAIL;
	}

	int offset= this->slot[sno].offset;
	memmove(recPtr, data+offset, recLen);
	return OK;
}

// **********************************************************
// returns length and pointer to record with RID rid.  The difference
// between this and getRecord is that getRecord copies out the record
// into recPtr, while this function returns a pointer to the record
// in recPtr.
Status HFPage::returnRecord(RID rid, char *&recPtr, int &recLen)
{
	int sno = rid.slotNo;

	if(sno > slotCnt || sno < 0) {
		return DONE;
	}
	recPtr = data + slot[sno].offset;
	recLen = slot[sno].length;

	return OK;
}
// Returns the amount of available space on the heap file page
int HFPage::available_space(void)
{
	// fill in the body
	return this->freeSpace - sizeof(slot_t);
}

// **********************************************************
// Returns 1 if the HFPage is empty, and 0 otherwise.
// It scans the slot directory looking for a non-empty slot.
bool HFPage::empty(void)
{
	// fill in the body
	if (freeSpace == MAX_SPACE - DPFIXED - sizeof(slot_t) || slotCnt == 0)
	return true;

	return false;
}
